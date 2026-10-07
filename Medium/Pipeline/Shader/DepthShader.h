///
/// @file   DepthShader.h
/// @author Garfield949
/// @date   2026/3/17 16:38
/// @brief  
///
#pragma once

#include "Pipeline/Shader/ShaderTypes.h"
#include "Resources/Vertex.h"
#include "Resources/Material.h"

namespace Medium {

struct ShaderBindContext;

struct DepthShader {
  struct Uniform {
    Mat4f mat_wvp_light;
    const Material* material = nullptr;
  };

  static constexpr u32 kVaryingScalarCount = kOffsetUV + 2;
  
  void bind(const ShaderBindContext& bind_ctx, const Mat4f& world, const Material* mat) noexcept;

  [[nodiscard]]
  VSOut vertex(const Vertex& vert) const noexcept {
    VSOut vs_out;
    // homogeneous 方法扩展 w = 1，经正交投影变换后的 pos_clip.w 仍为 1，走透视投影插值即退化为线性插值。
    vs_out.pos_clip = uniforms.mat_wvp_light * vert.pos.homogeneous();
    vs_out.varying.store(vert.uv.data(), kOffsetUV, 2);
    return vs_out;
  }

  [[nodiscard]]
  bool fragment(const FSIn& fs_in, LinearColor&) const noexcept {
    Vec2f frag_uv = fs_in.getVec2f(kOffsetUV);
  
    const Material* mat = uniforms.material;
  
    // Alpha Test
    if(mat->surface_mode == Material::SurfaceMode::Masked) {
      f32 value = sampleSlot(mat->diff_slot, frag_uv, fs_in.duv_dx, fs_in.duv_dy).A;
      if(value < Material::kAlphaCutoff) return true;
    }
  
    return false;
  }

  Uniform uniforms;
};

} // Medium