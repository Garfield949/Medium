///
/// @file   RotationMatrix.h
/// @author Garfield949
/// @date   2026/4/11 15:02
/// @brief  
///
#pragma once

#include "Base/Types.h"

namespace Medium {

  class RotationMatrix {
  public:
    static Mat3f makeFromForward(const Vec3f& forward, const Vec3f& up_ref = Vec3f::UnitY());
  };

} // Medium
