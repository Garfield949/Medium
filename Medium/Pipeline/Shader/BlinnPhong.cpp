///
/// @file   BlinnPhong.cpp
/// @author Garfield949
/// @date   2026/3/6 09:09
/// @brief  
///
#include "BlinnPhong.h"

#include "Scene/Camera.h"
#include "Pipeline/RenderContext.h"
#include "Scene/Light/DirectionalLight.h"

namespace Medium {
  
  void BlinnPhongShader::bind(const PassContext& ctx, const Mat4f& world, const Material* mat) noexcept {
    const Camera* camera = ctx.camera;
    FrameContext* ctx_frame = ctx.ctx_frame;
    
    uniforms.world      = world;
    uniforms.wvp        = camera->vpMatrix() * world;
    uniforms.camera_pos = camera->transform.worldPosition();
    uniforms.material   = mat;

    uniforms.shadow_map = ctx_frame->shadow_map;
    uniforms.wvp_light  = ctx_frame->main_light->vpMatrix();
    uniforms.light_dir  = -(ctx_frame->main_light->transform.forward());
  }
  
} // namespace Medium
