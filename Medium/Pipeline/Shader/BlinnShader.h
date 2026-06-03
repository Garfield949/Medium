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
#include "Pipeline/Shader/ShaderTypes.h"
#include "Pipeline/Shadow/ShadowQuery.h"
#include "Pipeline/Shadow/CSM.h"

namespace Medium {

struct ShaderBindCtx;

struct BlinnShader {
  struct Uniform {
    Vec3f camera_pos; // 用于高光
    Mat4f view;       // 用于阴影级联选择
    Mat4f wvp;

    Mat4f world;
    const Material*        material = nullptr;

    Vec3f light_dir_inv;
    const CSMSamplingCtx* csm_ctx = nullptr;
  };
  Uniform uniforms;
  
  void bind(const ShaderBindCtx& bind_ctx, const Mat4f& world, const Material* mat) noexcept;

  VSOut vertex(const Vertex& vert) const noexcept {
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

  bool fragment(const std::array<Varying, 3>& varyings,
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
    // if(mat->enable_diss) {
    //   f32 d = sampleSlot(mat->diss_slot, frag_uv).R;
    //   // 透明度阈值，cut off 是切断、剪切的意思。
    //   f32 alpha_cutoff = 0.5f;
    //   if(d < alpha_cutoff) return true;
    // }

    LinearColor diffuse = sampleSlot(mat->diff_slot, frag_uv);

    // N dot L, 无 Normal Mapping 扰动。
    const f32 nl = frag_n.dot(uniforms.light_dir_inv);
    
    // 该值为 1.0 代表当前 frag 不在阴影当中。
    f32 frag_visibility = 1.0f;
    if(nl <= 0.0f) {
      frag_visibility = 0.3f; // 背对光源则直接判定处于阴影中。
    } else {
      f32 frag_z_view = (uniforms.view * frag_pos.homogeneous()).z();
      frag_visibility = CSMBlend(uniforms.csm_ctx, nl, frag_pos, frag_n, frag_z_view);
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

    // 若启用 Normal Mapping, 则 nl_shading 会自动被扰动。
    const f32 nl_shading = frag_n.dot(uniforms.light_dir_inv);

    LinearColor specular{0};
    if(nl > 0 && mat->enable_spec) {
      Vec3f view_dir = (uniforms.camera_pos - frag_pos).normalized();

      Vec3f h = (view_dir + uniforms.light_dir_inv).normalized();

      f32 spec = std::powf(std::max(frag_n.dot(h), 0.0f), mat->params.Ns) * frag_visibility;

      specular = sampleSlot(mat->spec_slot, frag_uv) * spec;
    }

    // 半兰伯特：不受光的 frag 漫反射强度从 [-1,1] 被 wrap 到 [0.5,1] 再到 [0.25,1].
    f32 half_lambert = std::max(nl_shading, 0.0f) * 0.5f + 0.5f;

    f32 diff = half_lambert * half_lambert;

    frag_color = (diffuse * diff + specular) * frag_visibility;

    return false;
  }
};

} // namespace Medium