///
/// @file BlinnPhong.h
/// @brief 着色器
///
#pragma once

#include "Resources/Texture.h"
#include "fmt/base.h"

#include "Base/Types.h"
#include "Resources/Vertex.h"
#include "Resources/Material.h"
#include "Varying.h"
#include "Pipeline/RenderTarget.h"

namespace Medium {
  
  struct PassContext;

  struct BlinnPhongShader {
    struct Uniform {
      Mat4f world;
      Mat4f wvp;
      Vec3f camera_pos; // 用于高光着色
      const Material* material = nullptr;

      Mat4f wvp_light;
      Vec3f light_dir;
      const RenderTarget* shadow_map = nullptr;
    };
    
    Uniform uniforms;
    
    void bind(const PassContext& context, const Mat4f& world, const Material* mat) noexcept;

    Vec4f vertex(const Vertex& vert, Varying& varying) noexcept {
      Vec4f pos_clip  = uniforms.wvp * vert.pos.homogeneous();
      Mat3f world_rot = uniforms.world.topLeftCorner(3, 3);

      // 维护当前顶点的 varying. 只表示方向的向量变换到世界空间只需应用旋转
      varying.uv         = vert.uv;
      varying.normal     = (world_rot * vert.normal).normalized();
      varying.tangent   << (world_rot * vert.tangent.head<3>()).normalized(), vert.tangent.w();
      varying.pos_world  = (uniforms.world * vert.pos.homogeneous()).head(3);
      varying.w_inv      = 1.0f / pos_clip.w();

      return pos_clip;
    }

    bool fragment(const std::array<Varying, 3>& varyings, const Vec3f& weights, LinearColor& color_final) noexcept {
      Vec3f w_invs{varyings[0].w_inv, varyings[1].w_inv, varyings[2].w_inv};

      Vec3f n_interp  = Math::interpolate(varyings[0].normal, varyings[1].normal, varyings[2].normal, 
                                          weights, w_invs).normalized();

      Vec2f uv_interp = Math::interpolate(varyings[0].uv, varyings[1].uv, varyings[2].uv, weights, w_invs);

      Vec3f pos_world_interp = Math::interpolate(varyings[0].pos_world, varyings[1].pos_world, varyings[2].pos_world,
                                                   weights, w_invs);

      const Material* mat = uniforms.material;

      LinearColor diff_color = sampleSlot(mat->diff_slot, uv_interp);
      LinearColor spec_color{0.0f};
      LinearColor diss_color{1.0f};
      LinearColor norm_color{0.0f};
      
      if(mat->enable_diss) {
        // diss_color = sampleSlot(mat->diss_slot, uv_interp);
        if(diff_color.A < 0.5) return true;
      }

      if(mat->enable_norm) {
        Vec4f T_interp{Math::interpolate(varyings[0].tangent, varyings[1].tangent, varyings[2].tangent,
                                         weights, w_invs)};

        Vec3f T = T_interp.head<3>();

        // 施密特正交化
        T = (T - T.dot(n_interp) * n_interp).normalized();

        // T 和 N 均已在顶点着色器中应用了 world 矩阵，B 自然也附带了 world 的变换效果
        // 同一面片上顶点的 det 一定相同，所以插值出来的顶点的 det 可以直接取 varyings 其中之一的 w
        Vec3f B = T.cross(n_interp).normalized() * varyings[0].tangent.w();

        // 此时 TBN 同时具备从 tangent -> object 和 object -> world 的变换
        Mat3f TBN = (Mat3f() << T, B, n_interp).finished();

        norm_color = mat->norm_slot->sample(uv_interp, Texture::FilterMode::Bilinear);

        // 将 [0, 1] 的 LinearColor 映射到 [-1, 1]] 的坐标空间
        Vec3f normal_tangent = {norm_color.R * 2.0f - 1.0f, norm_color.G * 2.0f - 1.0f, norm_color.B * 2.0f - 1.0f};

        n_interp = TBN * normal_tangent.normalized();
      }

      if(mat->enable_spec) {
        Vec3f view_dir = (uniforms.camera_pos - pos_world_interp).normalized();

        Vec3f half_way = (view_dir + uniforms.light_dir).normalized();

        f32 spec_intens = std::powf(std::max(n_interp.dot(half_way), 0.0f), mat->params.Ns);

        spec_color = (sampleSlot(mat->spec_slot, uv_interp) * spec_intens);
      }

      f32 shadow_visibility = 1.0f;
      if(uniforms.shadow_map) {
        Vec4f light_clip = uniforms.wvp_light * pos_world_interp.homogeneous();
        Vec4f light_ndc  = light_clip / light_clip.w();

        Vec2f uv_shadow{(light_ndc.x() + 1) * 0.5f, (light_ndc.y() + 1) * 0.5f};

        if(uv_shadow.x() >= 0.0f && uv_shadow.x() <= 1.0f && uv_shadow.y() >= 0.0f && uv_shadow.y() <= 1.0f) {
          f32 z_map = uniforms.shadow_map->sampleDepth(uv_shadow);
          f32 z_cur = light_ndc.z();
          f32 bias = std::max(0.005f * (1.0f - n_interp.dot(uniforms.light_dir)), 0.001f);
          if(z_cur < z_map - bias) shadow_visibility = 0.3f;
        }
      }
      
      f32 diff_intens = std::max(n_interp.dot(uniforms.light_dir), 0.0f);
      
      f32 half_lambert = diff_intens * 0.5f + 0.5f;
      diff_intens = half_lambert * half_lambert;
      
      color_final = (diff_color * diff_intens + spec_color) * shadow_visibility;

      return false;
    }

  };

} // namespace Medium
