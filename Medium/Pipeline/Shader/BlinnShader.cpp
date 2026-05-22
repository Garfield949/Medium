///
/// @file   BlinnShader.cpp
/// @author Garfield949
/// @date   2026/3/6 09:09
/// @brief  
///
#include "BlinnShader.h"

#include "Pipeline/RenderContext.h"
#include "Scene/Light/DirectionalLight.h"

namespace Medium {
  
void BlinnShader::bind(const PassCtx& ctx, const Mat4f& world, const Material* mat) noexcept {
  const FrameCtx* frame_ctx = ctx.frame_ctx;
  const ViewCtx*  view_ctx  = ctx.view_ctx;
  
  uniforms.world      = world;
  uniforms.wvp        = view_ctx->vp * world;
  uniforms.camera_pos = view_ctx->cam_pos;
  uniforms.material   = mat;

  uniforms.shadow_map = frame_ctx->shadow_map;
  uniforms.wvp_light  = frame_ctx->main_light->vpMatrix();
  uniforms.light_dir  = -(frame_ctx->main_light->transform.forward());
  uniforms.dir_light_shadow_texel_size_ws = frame_ctx->dir_light_shadow_texel_size_ws;
}

} // namespace Medium
