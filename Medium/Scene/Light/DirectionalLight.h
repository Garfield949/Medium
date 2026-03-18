///
/// @file   DirectionalLight.h
/// @author Garfield949
/// @date   2026/3/15 23:38
/// @brief  
///
#pragma once

#include "Light.h"

namespace Medium {

  class Camera;

  class DirectionalLight : public Light {
  public:
    explicit DirectionalLight(std::string&& name);

    void setOrthoParams(f32 near, f32 far, f32 width, f32 height) const noexcept;

    [[nodiscard]] Mat4f matrixVP() const noexcept;

  public:
    Camera* ortho_camera_ = nullptr;
  };

} // namespace Medium
