///
/// @file   BlinnShader.cpp
/// @author Garfield949
/// @date   2026/3/6 09:09
/// @brief  
///
#include "BlinnShader.h"

#include "Pipeline/RenderContext.h"

namespace Medium {
  
void BlinnShader::bind(const ShaderBindCtx& bind_ctx, const Mat4f& world, const Material* mat) noexcept {
  const CameraCtx*   cam_ctx      = bind_ctx.camera_ctx;
  const LightingCtx* lighting_ctx = bind_ctx.lighting_ctx;

  this->uniforms = {
    .camera_pos    = cam_ctx->cam_pos,
    .view          = cam_ctx->view,
    .wvp           = cam_ctx->vp * world,

    .world         = world,
    .material      = mat,

    .light_dir_inv = -lighting_ctx->light_dir,
    .csm_ctx      =  lighting_ctx->csm_data
  };
}

} // namespace Medium
