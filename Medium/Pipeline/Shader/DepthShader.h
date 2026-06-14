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

struct ShaderBindCtx;

struct DepthShader {
  struct Uniform {
    Mat4f wvp_light;
    const Material* material = nullptr;
  };
  
  Uniform uniforms;
  
  void bind(const ShaderBindCtx& bind_ctx, const Mat4f& world, const Material* mat) noexcept;

  VSOut vertex(const Vertex& vert) const noexcept {
    Vec4f pos_clip = uniforms.wvp_light * vert.pos.homogeneous();

    Varying varying;
    varying.uv = vert.uv;

    return { pos_clip, varying };
  }

  bool fragment(const std::array<Varying, 3>& varyings,
                const Vec3f& weights,
                LinearColor&) const noexcept
  {
    Vec2f frag_uv = Math::interpolate(varyings[0].uv, varyings[1].uv, varyings[2].uv, weights);

    const Material* mat = uniforms.material;

    // Alpha Test
    if(mat->surface_mode == Material::SurfaceMode::Masked) {
      f32 value = sampleSlot(mat->diss_slot, frag_uv).R;
      if(value < Material::kAlphaCutoff) return true;
    }

    return false;
  }

};

} // Medium