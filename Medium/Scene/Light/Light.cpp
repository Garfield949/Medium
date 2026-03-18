///
/// @file   Light.cpp
/// @author Garfield949
/// @date   2026/3/15 23:24
/// @brief  
///
#include "Light.h"

namespace Medium {

  Light::Light(std::string&& name) : Node{name} {}

  void Light::setColor(const LinearColor& color) noexcept {
    this->color_ = color;
  }

  void Light::setIntensity(f32 intensity) noexcept {
    this->intensity_ = intensity;
  }
  
  void Light::castShadows(bool enable) noexcept {
    this->cast_shadows = enable;
  }

} // namespace Medium
