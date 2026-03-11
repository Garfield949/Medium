///
/// @file   BlinnPhong.cpp
/// @author Garfield949
/// @date   2026/3/6 09:09
/// @brief  
///
#include "BlinnPhong.h"

#include "Scene/Camera.h"

namespace Medium {
  
  void BlinnPhongShader::bind(const RenderContext& context, const Mat4f& world, const Material *mat) noexcept {
    uniforms.world      = world;
    uniforms.wvp        = context.projection_view * world;
    uniforms.camera_pos = context.camera.transform.worldPosition();
    uniforms.light_dir  = Vec3f{0.0f, 1.0f, 1.0f}.normalized();
    uniforms.material   = mat;
  }
  
} // namespace Medium
