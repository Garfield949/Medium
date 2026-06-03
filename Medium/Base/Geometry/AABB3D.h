///
/// @file   AABB3D.h
/// @author Garfield949
/// @date   2026/5/30 21:29
/// @brief  
///

#pragma once

#include "Base/Types.h"

namespace Medium {

/// @struct AABB3D
/// @brief 3D 包围盒类型
struct AABB3D {
  AABB3D() { reset(); }

  bool valid() const noexcept {
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

  Vec3f center() const noexcept {
    return (min + max) / 2.0f;
  }

  /// @brief 依据传入的世界矩阵将自身转换为一个世界空间 AABB 并返回。
  AABB3D toWorldAABB(const Mat4f& mat_world) const noexcept;

  /// @brief 返回包围盒的 8 个轴对齐角点：Zmin -> Zmax, left -> right, top -> bottom.
  std::array<Vec3f, 8> corners() const noexcept;

  Vec3f min;
  Vec3f max;
};

} // namespace Medium
