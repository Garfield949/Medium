///
/// @file   Light.h
/// @author Garfield949
/// @date   2026/3/15 23:02
/// @brief  
///
#pragma once

#include "Scene/Node.h"
#include "Base/Color.h"

namespace Medium {

  class Light : public Node{
  public:
    static constexpr NodeType Type = NodeType::Light;

    explicit Light(std::string name) : Node{std::move(name), Type} {}

    void setColor(const LinearColor& color) noexcept {
      this->color_ = color;
    }

    void setIntensity(f32 intensity) noexcept {
      this->intensity_ = intensity;
    }

    void castShadows(bool enable) noexcept {
      this->cast_shadows = enable;
    }

  protected:
    LinearColor color_ = LinearColor::White();
    f32 intensity_     = 1.0f;
    bool cast_shadows  = true;
  };

} // namespace Medium
