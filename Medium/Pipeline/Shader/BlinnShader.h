///
/// @file BlinnShader.h
/// @brief 布林冯着色器
///
#pragma once

#include <fmt/base.h>

#include "Base/Color.h"
#include "Base/Math/Math.h"
#include "Resources/Vertex.h"
#include "Resources/Material.h"
#include "Pipeline/RenderTarget.h"
#include "Pipeline/Shader/ShaderTypes.h"
#include "Pipeline/Shadow/ShadowQuery.h"

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
    f32   dir_light_shadow_texel_size_ws;
    const RenderTarget* shadow_map = nullptr;
  };
  Uniform uniforms;
  
  void bind(const PassCtx& context, const Mat4f& world, const Material* mat) noexcept;

  [[nodiscard]] VSOut vertex(const Vertex& vert) noexcept {
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

  [[nodiscard]] bool fragment(const std::array<Varying, 3>& varyings,
                              const Vec3f& weights,
                              LinearColor& frag_color) const noexcept
  {
    Vec3f w_invs{varyings[0].w_inv, varyings[1].w_inv, varyings[2].w_inv};

    // 注：frag_n 为几何法线 Geometry Normal.
    Vec2f frag_uv  = Math::interpolate(varyings[0].uv, varyings[1].uv, varyings[2].uv, weights, w_invs);
    Vec3f frag_n   = Math::interpolate(varyings[0].normal, varyings[1].normal, varyings[2].normal, weights, w_invs).normalized();
    Vec3f frag_pos = Math::interpolate(varyings[0].pos_world, varyings[1].pos_world, varyings[2].pos_world, weights, w_invs);

    const Material* mat = uniforms.material;

    // Alpha Test
    // TODO: 将 alpha_cutoff 设为一个材质参数或者一个全局常量
    if(mat->enable_diss) {
      f32 d = sampleSlot(mat->diss_slot, frag_uv).R;
      f32 alpha_cutoff = 0.5f; // 透明度阈值
      if(d < alpha_cutoff) return true;
    }

    LinearColor diffuse = sampleSlot(mat->diff_slot, frag_uv);

    // N dot L, 无 Normal Mapping 扰动。
    const f32 nl = frag_n.dot(uniforms.light_dir);

    // 该值为 1.0 代表：未启用 Shadow Mapping 或 当前 frag 不在阴影当中。
    f32 shadow = 1.0f;
    if(uniforms.shadow_map) {
      // 重建当前 frag 的光空间深度
      if(nl > 0.0f) {
        Vec3f frag_pos_biased = frag_pos +
          calcNormalBias(frag_n, nl, uniforms.dir_light_shadow_texel_size_ws);

        // 当前 frag 变换到光 clip 空间
        Vec4f frag_clip_ls = uniforms.wvp_light * frag_pos_biased.homogeneous();

        // 进而转为光 NDC 坐标
        Vec4f frag_ndc_ls  = frag_clip_ls / frag_clip_ls.w();

        // [-1,1] 的 NDC 坐标映射到 [0,1] 作为阴影采样 uv
        Vec2f uv{(frag_ndc_ls.x() + 1) * 0.5f, (frag_ndc_ls.y() + 1) * 0.5f};

        // 不在光视椎 near -> far 范围内的 frag 不参与 shadow compare;
        // uv 任意分量超出 [0,1] 说明不在光视椎长宽范围内，不参与 shadow compare.
        if(Math::isBetween(frag_ndc_ls.z(), 0.0f, 1.0f) && 
           Math::isBetween(uv.x(), 0.0f, 1.0f) && 
           Math::isBetween(uv.y(), 0.0f, 1.0f)) 
        {
          f32 depth_frag = frag_ndc_ls.z();
          shadow = sampleShadowPCF(uniforms.shadow_map, uv, depth_frag);
        }
      } else {
        // 背部不受光自然处于阴影中
        shadow = 0.3f;
      }
    }

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

      // 这里 frag_n 会被赋值为贴图采样的法向，为着色法线 Shading Normal，以扰动后续的所有光照计算。
      frag_n = TBN * n_T;
    }

    // 若启用 Normal Mapping, 则 nol_shading 会自动被扰动。
    const f32 nl_shading = frag_n.dot(uniforms.light_dir);

    LinearColor specular{0};
    if(nl > 0 && mat->enable_spec && shadow == 1.0f) {
      Vec3f view_dir = (uniforms.camera_pos - frag_pos).normalized();

      Vec3f h = (view_dir + uniforms.light_dir).normalized();

      f32 spec = std::powf(std::max(frag_n.dot(h), 0.0f), mat->params.Ns);

      specular = sampleSlot(mat->spec_slot, frag_uv) * spec;
    }

    // 半兰伯特：不受光的 frag(nl <= 0) 的漫反射强度从 [-1,1] 被 wrap 到 [0,1]，某种程度代替了环境光。
    f32 half_lambert = nl_shading * 0.5f + 0.5f;

    f32 diff = half_lambert * half_lambert;

    frag_color = (diffuse * diff + specular) * shadow;

    return false;
  }
};

} // namespace Medium