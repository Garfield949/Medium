///
/// @file   ShadowShader.cpp
/// @author Garfield949
/// @date   2026/3/17 16:38
/// @brief  
///
#include "ShadowShader.h"

#include "Pipeline/RenderContext.h"
#include "Scene/Light/DirectionalLight.h"

namespace Medium {
  
  void ShadowShader::bind(const RenderContext& context, const Mat4f& world, const Material* mat) noexcept {
    uniforms.wvp_light = context.light->matrixVP() * world;
    uniforms.material  = mat;
  }

} // Medium