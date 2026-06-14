///
/// @file   DepthShader.cpp
/// @author Garfield949
/// @date   2026/3/17 16:38
/// @brief  
///
#include "DepthShader.h"

#include "Pipeline/RenderContext.h"

namespace Medium {
  
void DepthShader::bind(const ShaderBindCtx& bind_ctx, const Mat4f& world, const Material* mat) noexcept {
  const CameraCtx* cam_ctx = bind_ctx.camera_ctx;

  this->uniforms = {
    .wvp_light = cam_ctx->vp * world,
    .material  = mat,
  };
}

} // Medium