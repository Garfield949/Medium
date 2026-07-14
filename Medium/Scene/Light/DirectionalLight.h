///
/// @file   DirectionalLight.h
/// @author Garfield949
/// @date   2026/3/15 23:38
/// @brief  
///
#pragma once

#include "Base/Types.h"
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

  void setOrthoParams(f32 far, f32 ortho_width, f32 ortho_height) const noexcept {
    ortho_camera_->setOrthoParams(far, ortho_width, ortho_height);
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

  Vec3f dir() const noexcept { return transform.forward(); }

  void setDir(const Vec3f& new_dir) noexcept {
    if(new_dir.isZero()) return;
    // 注意 lookAt 传的参数是一个点。
    transform.lookAt(transform.worldPosition() + new_dir.normalized());
  }

private:
  Camera* ortho_camera_ = nullptr;
};

} // namespace Medium
