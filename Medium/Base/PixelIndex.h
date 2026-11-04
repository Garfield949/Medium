///
/// @file   PixelIndex.h
/// @author Garfield949
/// @date   2026/11/3 10:13
/// @brief  
///
#pragma once

#include "Types.h"

namespace Medium::PixelIndex {

inline size_t get(i32 x, i32 y, i32 width) noexcept {
  return y * width + x;
}

inline size_t getFlipped(i32 x, i32 y, i32 width, i32 height) noexcept {
  return get(x, height - y - 1, width);
}

} // namespace Medium::PixelIndex
