///
/// @file   Rect.h
/// @author Garfield949
/// @date   2026/5/30 18:14
/// @brief  
///
#pragma once

#include "Base/Types.h"
#include <algorithm>

namespace Medium {

struct RectF {
  f32 x = 0.0f;
  f32 y = 0.0f;
  f32 w = 0.0f;
  f32 h = 0.0f;
};

/// @struct Rect
/// @brief 二维矩形
/// @details 通过原点 + 尺寸的形态遍历时对齐左闭右开语义；通过 min, max 遍历时对齐闭区间语义。
struct Rect {
  Rect() = default;

  Rect(i32 x, i32 y, i32 w, i32 h)
    : x{x}, y{y}, w{w}, h{h} {}
  
  Rect(const RectF& other) {
    x = static_cast<i32>(other.x);
    y = static_cast<i32>(other.y);
    w = static_cast<i32>(other.w);
    h = static_cast<i32>(other.h);
  } // 支持隐式转换

  bool valid() const noexcept {
    return w > 0 && h > 0;
  }

  /// @brief 返回起始像素的坐标。
  Vec2i min() const noexcept { return {x, y}; }

  /// @brief 返回末尾像素的坐标。
  Vec2i max() const noexcept { return {x + w - 1, y + h - 1}; }

  /// @brief 返回两个矩形的交集。
  Rect intersection(const Rect& other) const noexcept {
    i32 x_min = std::max(x, other.x);
    i32 y_min = std::max(y, other.y);
    i32 x_max = std::min(x + w - 1, other.x + other.w - 1);
    i32 y_max = std::min(y + h - 1, other.y + other.h - 1);
    return { x_min, y_min, x_max - x_min + 1, y_max - y_min + 1 };
  }

  i32 x = 0;
  i32 y = 0;
  i32 w = 0;
  i32 h = 0;
};

} // Medium
