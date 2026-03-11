///
/// @file BlinnPhong.h
/// @brief 着色器
///
#pragma once

#include "fmt/base.h"

#include "Base/Color.h"
#include "Base/Types.h"
#include "Resources/Vertex.h"
#include "Resources/Material.h"

namespace Medium {
  
  struct RenderContext;

  struct BlinnPhongShader {
    struct Uniform {
      Mat4f world;
      Mat4f wvp;
      Vec3f camera_pos; // 用于高光着色
      Vec3f light_dir;

      const Material* material;
    };
    
    struct Varying {
      Vec2f uv;        // 纹理坐标
      Vec3f normal;    // 顶点变到世界空间里的法向
      Vec4f tangent;   //
      Vec3f pos_world; // 某些计算（比如光照）依赖顶点世界坐标
      f32   w_inv;     // 保存顶点的 1/w
    };
    
    Uniform                uniforms;
    std::array<Varying, 3> varyings;
    
    void bind(const RenderContext& context, const Mat4f& world, const Material* mat) noexcept;

    Vec4f vertex(const u32 nth_vert, const Vertex& vert) noexcept {
      Vec4f pos_clip  = uniforms.wvp * vert.pos.homogeneous();
      Mat3f world_rot = uniforms.world.topLeftCorner(3, 3);

      f32 det = vert.tangent.w();

      // 维护当前顶点的 Varyings. 由于顶点法线只有方向，故变换到世界空间只需应用旋转。
      varyings[nth_vert].uv        = vert.uv;
      varyings[nth_vert].normal    = (world_rot * vert.normal).normalized();
      varyings[nth_vert].tangent    = (world_rot * vert.tangent.head<3>()).normalized().homogeneous();
      varyings[nth_vert].pos_world = (uniforms.world * vert.pos.homogeneous()).head(3);
      varyings[nth_vert].w_inv     = 1.0f / pos_clip.w();

      varyings[nth_vert].tangent.w() = det;

      return pos_clip;
    }

    bool fragment(const Vec3f& weights, LinearColor& color_final) noexcept {
      Vec3f w_invs{varyings[0].w_inv, varyings[1].w_inv, varyings[2].w_inv};

      Vec3f N_interp = Math::interpolate(varyings[0].normal,
                                         varyings[1].normal,
                                         varyings[2].normal, weights, w_invs).normalized();

      Vec2f UV_interp = Math::interpolate(varyings[0].uv,
                                          varyings[1].uv,
                                          varyings[2].uv, weights, w_invs);

      const Material* mat = uniforms.material;

      LinearColor diff_color = sampleSlot(mat->diff_slot, UV_interp);
      LinearColor spec_color{0.0f};
      LinearColor diss_color{1.0f};
      LinearColor norm_color{0.0f};
      
      if(mat->enable_diss) {
        // diss_color = sampleSlot(mat->diss_slot, uv_interp);
        if(diff_color.A < 0.5) return true;
      }

      if(mat->enable_norm) {
        Vec4f T_interp = Math::interpolate(varyings[0].tangent,
                                           varyings[1].tangent,
                                           varyings[2].tangent, weights, w_invs);

        f32 det = T_interp.w() < 0 ? -1.0f : 1.0f;

        Vec3f T = T_interp.head<3>();

        // 施密特正交化
        T = (T - T.dot(N_interp) * N_interp).normalized();

        Vec3f B = T.cross(N_interp).normalized() * det;

        Mat3f TBN = Mat3f::Zero();
        TBN.col(0) = T;
        TBN.col(1) = B;
        TBN.col(2) = N_interp;

        norm_color = mat->norm_slot->sample(UV_interp, FilterMode::Bilinear);

        // 将 [0, 1] 的 LinearColor 映射到 [-1, 1]] 的坐标空间
        Vec3f normal_tangent;
        normal_tangent.x() = norm_color.R * 2.0f - 1.0f;
        normal_tangent.y() = norm_color.G * 2.0f - 1.0f;
        normal_tangent.z() = norm_color.B * 2.0f - 1.0f;

        Vec3f normal_world = TBN * normal_tangent.normalized();

        N_interp = normal_world;
      }

      if(mat->enable_spec) {
        Vec3f pos_world_interp = Math::interpolate(varyings[0].pos_world,
                                                   varyings[1].pos_world,
                                                   varyings[2].pos_world, weights, w_invs);

        Vec3f view_dir = (uniforms.camera_pos - pos_world_interp).normalized();

        Vec3f half_way = (view_dir + uniforms.light_dir).normalized();

        f32 spec_intens = std::powf(std::max(N_interp.dot(half_way), 0.0f), mat->params.Ns);

        spec_color = (sampleSlot(mat->spec_slot, UV_interp) * spec_intens);
      }
      
      f32 diff_intens = std::max(N_interp.dot(uniforms.light_dir), 0.0f);
      
      f32 half_lambert = diff_intens * 0.5f + 0.5f;
      diff_intens = half_lambert * half_lambert;
      
      color_final = diff_color * diff_intens + spec_color;

      return false;
    }

  };

} // namespace Medium
