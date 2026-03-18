///
/// @file   ShadowMap.h
/// @author Garfield949
/// @date   2026/3/14 22:23
/// @brief  
///
#pragma once

#include "Base/Types.h"

namespace Medium {

  class ShadowMap {
  public:
    explicit ShadowMap(size_t resolution);

    [[nodiscard]] f32 sample(const Vec2f& uv) const noexcept {
      f32 u = std::clamp(uv.x(), 0.0f, 1.0f);
      f32 v = std::clamp(uv.y(), 0.0f, 1.0f);
    }

  private:
    size_t res_;
    std::vector<f32> depth_buf_;
  };

} // Medium