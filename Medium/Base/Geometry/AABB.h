///
/// @file   AABB.h
/// @author Garfield949
/// @date   2026/3/20 16:36
/// @brief  该文件定义包围盒类型
///
#pragma once

#include "Base/Types.h"
#include "Pipeline/Subpixel.h"
#include "Pipeline/Viewport.h"

namespace Medium {

/// @struct ScreenAABB
/// @brief 屏幕空间包围盒类型
struct ScreenAABB {
  ScreenAABB() = default;

  /// @brief 判断包围盒是否有效
  constexpr bool valid() const noexcept {
    return min.x() <= max.x() && min.y() <= max.y();
  }

  // Viewport 的作用是进行边界截断。
  static ScreenAABB fromSubpixelTri(const std::array<Vec2i, 3>& tri_subpixel,
                                    const Viewport& viewport) 
  {
    ScreenAABB bbox;

    i32 x_min = std::min({tri_subpixel[0].x(), tri_subpixel[1].x(), tri_subpixel[2].x()});
    i32 x_max = std::max({tri_subpixel[0].x(), tri_subpixel[1].x(), tri_subpixel[2].x()});
    i32 y_min = std::min({tri_subpixel[0].y(), tri_subpixel[1].y(), tri_subpixel[2].y()});
    i32 y_max = std::max({tri_subpixel[0].y(), tri_subpixel[1].y(), tri_subpixel[2].y()});

    // 将包围盒范围从 Subpixel 转到 Pixel 空间。进行安全的取整策略：宁可多框一些像素，避免漏掉边缘像素。
    // 1. Floor   行为：单纯右移。使得在 [160, 176) 范围内取整结果为 10；
    // 2. Ceiling 行为：加一个偏置再右移。这里加 15，使得在 (160, 176] 范围内取整结果为 11；
    bbox.min.x() = std::max(viewport.x, x_min >> Subpixel::kBits);
    bbox.min.y() = std::max(viewport.y, y_min >> Subpixel::kBits);
    bbox.max.x() = std::min(static_cast<i32>(viewport.x + viewport.w - 1), (x_max + Subpixel::kMask) >> Subpixel::kBits);
    bbox.max.y() = std::min(static_cast<i32>(viewport.y + viewport.h - 1), (y_max + Subpixel::kMask) >> Subpixel::kBits);

    return bbox;
  }

  // 使用 i32 是因为非法 BBOX 可能会有负值
  Vec2i min; // 左下角
  Vec2i max; // 右上角
};

} // namespace Medium