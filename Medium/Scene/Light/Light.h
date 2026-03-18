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
    explicit Light(std::string&& name);

    void setColor(const LinearColor& color) noexcept;
    void setIntensity(f32 intensity)        noexcept;
    void castShadows(bool enable)           noexcept;

  protected:
    LinearColor color_ = LinearColor::White();
    f32 intensity_     = 1.0f;
    bool cast_shadows  = true;
  };

} // namespace Medium
