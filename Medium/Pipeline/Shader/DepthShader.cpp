///
/// @file   DepthShader.cpp
/// @author Garfield949
/// @date   2026/3/17 16:38
/// @brief  
///
#include "DepthShader.h"

#include "Pipeline/RenderContext.h"
#include "Scene/Light/DirectionalLight.h"

namespace Medium {
  
void DepthShader::bind(const PassCtx& ctx, const Mat4f& world, const Material* mat) noexcept {
  uniforms.wvp_light = ctx.frame_ctx->main_light->vpMatrix() * world;
  uniforms.material  = mat;
}

} // Medium