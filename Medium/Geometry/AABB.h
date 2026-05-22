///
/// @file   AABB.h
/// @author Garfield949
/// @date   2026/3/20 16:36
/// @brief  该文件定义包围盒类型
///
#pragma once

#include "Base/Types.h"
#include "Geometry/Subpixel.h"

namespace Medium {

/// @struct AABB3D
/// @brief 3D 包围盒类型
struct AABB3D {
  AABB3D() {
    reset();
  }
  
  [[nodiscard]] constexpr bool valid() const noexcept {
    return min.x() <= max.x() && min.y() <= max.y() && min.z() <= max.z();
  }

  void reset() noexcept {
    min.fill(std::numeric_limits<f32>::max());
    max.fill(std::numeric_limits<f32>::lowest());
  }

  // cwise 系列是 Eigen 的逐系数 coefficient-wise 方法
  // 比方说，A = [10, 2, 5], B = [3, 8, 4]. 则 A.cwiseMin(B) = [3, 2, 4].
  void addPoint(const Vec3f& p) {
    min = min.cwiseMin(p);
    max = max.cwiseMax(p);
  }

  void merge(const AABB3D& other) noexcept {
    min = min.cwiseMin(other.min);
    max = max.cwiseMax(other.max);
  }

  [[nodiscard]] Vec3f center() const noexcept {
    return (min + max) / 2.0f;
  }
  
  // Arvo's Algorithm
  [[nodiscard]] AABB3D toWorldAABB(const Mat4f& mat_world) const noexcept {
    AABB3D bbox_world;
    bbox_world.min = bbox_world.max = mat_world.topRightCorner<3, 1>();

    for(i32 i = 0; i < 3; ++i) {
      for(i32 j = 0; j < 3; ++j) {
        f32 a = mat_world(i, j) * min(j);
        f32 b = mat_world(i, j) * max(j);
        if(a > b) {
          bbox_world.max(i) += a;
          bbox_world.min(i) += b;
        } else {
          bbox_world.max(i) += b;
          bbox_world.min(i) += a;
        }
      }
    }
    return bbox_world;
  }

  /// @brief 返回包围盒的 8 个角点
  /// @note 注意，角点是轴对齐的
  [[nodiscard]] std::array<Vec3f, 8> corners() const noexcept {
    f32 min_x = min.x(), min_y = min.y(), min_z = min.z();
    f32 max_x = max.x(), max_y = max.y(), max_z = max.z();
    return std::array<Vec3f, 8> {{
      {min_x, min_y, min_z}, {min_x, min_y, max_z}, {max_x, min_y, max_z}, {max_x, min_y, min_z},
      {min_x, max_y, min_z}, {min_x, max_y, max_z}, {max_x, max_y, max_z}, {max_x, max_y, min_z}
    }};
  }

  Vec3f min;
  Vec3f max;
};

/// @struct ScreenAABB
/// @brief 屏幕空间包围盒类型
struct ScreenAABB {
  ScreenAABB() = default;

  /// @brief 判断包围盒是否有效
  [[nodiscard]] constexpr bool valid() const noexcept {
    return min.x() <= max.x() && min.y() <= max.y();
  }

  static ScreenAABB fromSubpixelTri(const std::array<Vec2i, 3>& tri_subpixel,
                                                            u32 width_screen,
                                                            u32 height_screen) {
    ScreenAABB bbox;

    i32 x_min = std::min({tri_subpixel[0].x(), tri_subpixel[1].x(), tri_subpixel[2].x()});
    i32 x_max = std::max({tri_subpixel[0].x(), tri_subpixel[1].x(), tri_subpixel[2].x()});
    i32 y_min = std::min({tri_subpixel[0].y(), tri_subpixel[1].y(), tri_subpixel[2].y()});
    i32 y_max = std::max({tri_subpixel[0].y(), tri_subpixel[1].y(), tri_subpixel[2].y()});

    // 包围盒范围从 Subpixel 转到 Pixel 空间。进行安全的取整策略：宁可多框一些像素，避免漏掉边缘像素；
    // 1. Floor   行为：单纯右移。使得在 [160, 176) 范围内取整结果为 10；
    // 2. Ceiling 行为：加一个偏置再右移。这里加 15，使得在 (160, 176] 范围内取整结果为 11；
    bbox.min.x() = std::max(0, x_min >> Subpixel::Bits);
    bbox.min.y() = std::max(0, y_min >> Subpixel::Bits);
    bbox.max.x() = std::min(static_cast<i32>(width_screen  - 1), (x_max + Subpixel::Mask) >> Subpixel::Bits);
    bbox.max.y() = std::min(static_cast<i32>(height_screen - 1), (y_max + Subpixel::Mask) >> Subpixel::Bits);

    return bbox;
  }

  // 使用 i32 是因为非法 BBOX 可能会有负值
  Vec2i min; // 左下角
  Vec2i max; // 右上角
};

} // namespace Medium