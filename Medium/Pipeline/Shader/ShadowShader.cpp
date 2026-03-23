///
/// @file   ShadowShader.cpp
/// @author Garfield949
/// @date   2026/3/17 16:38
/// @brief  
///
#include "ShadowShader.h"

#include "Pipeline/RenderContext.h"
#include "Scene/Light/DirectionalLight.h"

#include <iostream>

namespace Medium {
  
  void ShadowShader::bind(const PassContext& context, const Mat4f& world, const Material* mat) noexcept {
    uniforms.wvp_light = context.ctx_frame->main_light->vpMatrix() * world;
    uniforms.material  = mat;
  }

} // Medium