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

  class DirectionalLight : public Light {
  public:
    explicit DirectionalLight(std::string name) : Light{std::move(name)} {
      ortho_camera_ =
      attachChild(std::make_unique<Camera>("Ortho Camera", Camera::Mode::Ortho));
    }

    void setOrthoParams(f32 near, f32 far, f32 width, f32 height) const noexcept {
      ortho_camera_->setOrthoParams(near, far, width, height);
    }

    [[nodiscard]] Mat4f vpMatrix() const noexcept {
      return ortho_camera_->vpMatrix();
    }

    [[nodiscard]] Mat4f viewMatrix() const noexcept {
      return ortho_camera_->viewMatrix();
    }

    [[nodiscard]] const Camera* attachedCamera() const noexcept {
      return ortho_camera_;
    }

    Vec3f dir;

  private:
    Camera* ortho_camera_ = nullptr;
  };

} // namespace Medium
