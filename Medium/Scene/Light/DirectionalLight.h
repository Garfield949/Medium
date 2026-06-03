///
/// @file   DirectionalLight.h
/// @author Garfield949
/// @date   2026/3/15 23:38
/// @brief  
///
#pragma once

#include "Light.h"
#include "Scene/Camera.h"

namespace Medium {

class Camera;
struct DirectionalViewFit;

class DirectionalLight : public Light {
public:
  explicit DirectionalLight(std::string name) : Light{std::move(name)} {
    ortho_camera_ =
    attachChild(std::make_unique<Camera>("Orthographic Camera", Camera::Mode::Orthographic));
  }

  void setOrthoParams(f32 near, f32 far, f32 width, f32 height) const noexcept {
    ortho_camera_->setOrthoParams(near, far, width, height);
  }

  Mat4f viewProjMatrix() const noexcept {
    return ortho_camera_->viewProjMatrix();
  }

  Mat4f viewMatrix() const noexcept {
    return ortho_camera_->viewMatrix();
  }

  const Camera* attachedCamera() const noexcept {
    return ortho_camera_;
  }

  void applyViewFit(const DirectionalViewFit& fit);

  Vec3f dir;

private:
  Camera* ortho_camera_ = nullptr;
};

} // namespace Medium
