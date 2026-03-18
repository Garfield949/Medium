///
/// @file   ShadowShader.h
/// @author Garfield949
/// @date   2026/3/17 16:38
/// @brief  
///
#pragma once

#include "Varying.h"
#include "Base/Types.h"
#include "Resources/Vertex.h"
#include "Resources/Material.h"

namespace Medium {

  struct RenderContext;

  struct ShadowShader {
    struct Uniform {
      Mat4f wvp_light;
      const Material* material = nullptr;
    };
    
    Uniform uniforms;
    
    void bind(const RenderContext& context, const Mat4f& world, const Material* mat) noexcept;

    Vec4f vertex(const Vertex& vert, Varying& varying) noexcept {
      varying.uv    = vert.uv;
      return uniforms.wvp_light * vert.pos.homogeneous();
    }

    bool fragment(const std::array<Varying, 3>& varyings, const Vec3f& weights, LinearColor& color_final) noexcept {
      Vec2f uv_interp = Math::interpolate(varyings[0].uv, varyings[1].uv, varyings[2].uv, weights);

      const Material* mat = uniforms.material;
      
      if(mat->enable_diss && sampleSlot(mat->diss_slot, uv_interp).R < 0.5f)
        return true;
      
      return false;
    }
  };

} // Medium