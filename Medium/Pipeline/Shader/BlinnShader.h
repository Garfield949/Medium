///
/// @file BlinnShader.h
/// @brief 布林冯着色器
///
#pragma once

#include <fmt/base.h>

#include "Base/Color.h"
#include "Resources/Vertex.h"
#include "Resources/Material.h"
#include "Pipeline/RenderTarget.h"
#include "Pipeline/Shader/Common.h"

namespace Medium {
  
  struct PassCtx;

  struct BlinnShader {
    struct Uniform {
      Mat4f world;
      Mat4f wvp;
      Vec3f camera_pos; // 用于高光
      const Material* material = nullptr;

      Mat4f wvp_light;
      Vec3f light_dir;
      const RenderTarget* shadow_map = nullptr;
    };
    
    Uniform uniforms;
    
    void bind(const PassCtx& context, const Mat4f& world, const Material* mat) noexcept;

    VSOut vertex(const Vertex& vert) noexcept {
      Vec4f pos_clip  = uniforms.wvp * vert.pos.homogeneous();
      Mat3f rot_world = uniforms.world.topLeftCorner(3, 3);

      // 维护当前顶点的 varying. 只表示方向的向量变换到世界空间只需应用旋转
      Varying varying;
      varying.uv         = vert.uv;
      varying.normal     = (rot_world * vert.normal).normalized();
      varying.tangent   << (rot_world * vert.tangent.head<3>()).normalized(), vert.tangent.w();
      varying.pos_world  = (uniforms.world * vert.pos.homogeneous()).head(3);
      varying.w_inv      = 1.0f / pos_clip.w();

      return { pos_clip, varying };
    }

    bool fragment(const std::array<Varying, 3>& varyings, const Vec3f& weights, LinearColor& frag_color) const noexcept {
      Vec3f w_invs{varyings[0].w_inv, varyings[1].w_inv, varyings[2].w_inv};

      Vec2f frag_uv  = Math::interpolate(varyings[0].uv, varyings[1].uv, varyings[2].uv, weights, w_invs);
      Vec3f frag_n   = Math::interpolate(varyings[0].normal, varyings[1].normal, varyings[2].normal, weights, w_invs).normalized();
      Vec3f frag_pos = Math::interpolate(varyings[0].pos_world, varyings[1].pos_world, varyings[2].pos_world, weights, w_invs);

      const Material* mat = uniforms.material;

      // Alpha Test
      if(mat->enable_diss) {
        f32 d = sampleSlot(mat->diss_slot, frag_uv).R;
        if(d < 0.5) return true;
      }

      LinearColor diffuse = sampleSlot(mat->diff_slot, frag_uv);

      // Normal Mapping
      if(mat->enable_norm) {
        Vec3f T = Math::interpolate(varyings[0].tangent, varyings[1].tangent, varyings[2].tangent,
                                    weights, w_invs).head<3>();
        // 施密特正交化
        T = (T - T.dot(frag_n) * frag_n).normalized();

        // T 和 N 均已在顶点着色器中应用了 world 矩阵，B 自然也附带了 world 的变换效果
        // 同一面片上顶点的 det 一定相同，所以插值出来的顶点的 det 可以直接取 varyings 其中之一的 w
        Vec3f B = T.cross(frag_n).normalized() * varyings[0].tangent.w();

        // 此时 TBN 同时具备从 tangent -> object 和 object -> world 的变换
        Mat3f TBN = (Mat3f() << T, B, frag_n).finished();

        LinearColor raw = mat->norm_slot->sample(frag_uv, Texture::FilterMode::Bilinear);

        // 将 [0, 1] 的 LinearColor 映射到 [-1, 1]] 的坐标空间，得到切线空间的法向
        Vec3f n_T = Vec3f{raw.R * 2.0f - 1.0f, raw.G * 2.0f - 1.0f, raw.B * 2.0f - 1.0f}.normalized();

        frag_n = TBN * n_T;
      }

      LinearColor specular;
      if(mat->enable_spec) {
        Vec3f view_dir = (uniforms.camera_pos - frag_pos).normalized();

        Vec3f h = (view_dir + uniforms.light_dir).normalized();

        f32 spec = std::powf(std::max(frag_n.dot(h), 0.0f), mat->params.Ns);

        specular = sampleSlot(mat->spec_slot, frag_uv) * spec;
      }

      f32 shadow = 1.0f;
      if(uniforms.shadow_map) {
        Vec4f pos_light_clip = uniforms.wvp_light * frag_pos.homogeneous();

        Vec4f ndc_coords  = pos_light_clip / pos_light_clip.w();

        Vec2f uv{(ndc_coords.x() + 1) * 0.5f, (ndc_coords.y() + 1) * 0.5f};

        if(Math::between(uv.x(), 0.0f, 1.0f) && Math::between(uv.y(), 0.0f, 1.0f)) {
          f32 z_map = uniforms.shadow_map->sampleDepth(uv);
          f32 z_cur = ndc_coords.z();
          // 1. 自影伪影问题：Shadow Map 的分辨率不足，场景中任意一个表面区域附近可能会有若干个点映射到阴影纹理
          //    的同一个深度值上，导致在主 Pass 查询深度时，当前点的实际深度大于在纹理中记录的值，造成“自己遮挡自
          //    己”的视觉错误；
          // 2. 光线方向与表面越倾斜，误差越大：想象一下，此时光源视野里的一块正方形区域投射至斜面上时，会覆盖更长
          //    的物理距离，使得单位区域里有更多点被映射到同一个深度上；
          // 3. 所以需计算一个偏置值 bias，将阴影纹理记录的深度稍微变大一点，手动让产生误差的计算躲开临界点。这里
          //    将 bias 控制在 [0.001, 0.005] 之间，光线与表面的夹角越大，则 bias 越大，符合语义；
          // 4. 注意彼得潘现象：加上 bias 的本质是将光源视野里的最近深度往更深处推。假设有一个较薄的物体挡住地面产
          //    生阴影，由于 bias 过大，该物体更靠近地面一端的深度被推到地面下面了，会导致该端下方的地面被认为是受
          //    光的，进而使阴影起始点错位。
          f32 bias = std::max(0.005f * (1.0f - frag_n.dot(uniforms.light_dir)), 0.001f);
          if(z_cur < z_map - bias) shadow = 0.3f;
        }
      }
      
      f32 diff = std::max(frag_n.dot(uniforms.light_dir), 0.0f);
      
      f32 half_lambert = diff * 0.5f + 0.5f;
      diff = half_lambert * half_lambert;
      
      frag_color = (diffuse * diff + specular) * shadow;

      return false;
    }

  };

} // namespace Medium
