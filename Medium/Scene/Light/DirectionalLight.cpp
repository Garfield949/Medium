///
/// @file   DirectionalLight.cpp
/// @author Garfield949
/// @date   2026/3/16 19:44
/// @brief  
///
#include "DirectionalLight.h"

#include "Scene/Camera.h"

namespace Medium {

  DirectionalLight::DirectionalLight(std::string&& name) : Light{std::move(name)} {
    ortho_camera_ =
    attachChild(std::make_unique<Camera>("Ortho Camera", ProjectionMode::Ortho));
  }

  void DirectionalLight::setOrthoParams(f32 near, f32 far, f32 width, f32 height) const noexcept {
    ortho_camera_->setOrthoParams(near, far, width, height);
  }

  Mat4f DirectionalLight::matrixVP() const noexcept {
    return ortho_camera_->matrixVP();
  }

} // namespace Medium
