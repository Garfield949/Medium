///
/// @file   Util.h
/// @author Garfield949
/// @date   2026/3/6 13:53
/// @brief  
///
#pragma once

#include "Types.h"

namespace Medium::Util {

  inline u8 quantizeToU8(f32 value) {
    return static_cast<u8>(value * 255.0f + 0.5f);
  }

} // namespace Medium
