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
}

namespace Medium {

struct DepthShader {
  struct Uniform {
    Mat4f wvp_light;
    const Material* material = nullptr;
  };
  
  static constexpr u32 kOffsetUV = 0;  // Vec2f

  static constexpr std::array<VaryingField, 1> kVaryingFields = {{
    {.offset = kOffsetUV, .scalar_cnt = 2, .interp_mode = InterpMode::NoPerspective}
  }};

  static constexpr u32 varyingScalarCount() noexcept { return kOffsetUV + 2; }

  static constexpr VaryingLayout varyingLayout() noexcept { 
    return {
      .fields = kVaryingFields,
      .scalar_cnt = varyingScalarCount()
    };
  }
  
  void bind(const ShaderBindCtx& bind_ctx, const Mat4f& world, const Material* mat) noexcept;

  [[nodiscard]]
  VSOut vertex(const Vertex& vert) const noexcept {
    VSOut vs_out;
    vs_out.pos_clip = uniforms.wvp_light * vert.pos.homogeneous();
    vs_out.varying.w_inv = 1.0f; // 走透视投影插值也可以，w 设为 1 就退化为线性插值了。
    vs_out.varying.store(vert.uv.data(), kOffsetUV, 2);
    
    return vs_out;
  }

  [[nodiscard]]
  bool fragment(const FSPayload& fs_payload) const noexcept {
    Vec2f frag_uv = fs_payload.getVec2f(kOffsetUV);
  
    const Material* mat = uniforms.material;
  
    // Alpha Test
    if(mat->surface_mode == Material::SurfaceMode::Masked) {
      f32 value = sampleSlot(mat->diss_slot, frag_uv).R;
      if(value < Material::kAlphaCutoff) return true;
    }
  
    return false;
  }

  Uniform uniforms;
};

} // Medium