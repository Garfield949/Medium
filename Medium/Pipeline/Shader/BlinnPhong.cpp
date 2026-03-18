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
  
  void BlinnPhongShader::bind(const RenderContext& context, const Mat4f& world, const Material* mat) noexcept {
    uniforms.world      = world;
    uniforms.wvp        = context.main_camera->matrixVP() * world;
    uniforms.camera_pos = context.main_camera->transform.worldPosition();
    uniforms.material   = mat;

    uniforms.shadow_map = context.shadow_map;
    uniforms.wvp_light  = context.light->matrixVP();
    uniforms.light_dir  = -context.light->transform.forward();
  }
  
} // namespace Medium
