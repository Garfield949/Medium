///
/// @file   Sphere.h
/// @author Garfield949
/// @date   2026/3/31 15:00
/// @brief
///
#pragma once

#include "Base/Types.h"

namespace Medium {

struct Sphere {
  enum class Approach {
    Fast, Exact
  };

  Vec3f center{0};
    f32 radius{0};
};

} // namespace Medium
