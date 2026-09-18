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
  /// @brief 计算给定视椎的外接球
  /// @param corners 接收视椎的角点
  /// @return 返回值 Vec2f 的 x 为球心坐标，y 为球半径
  /// @details 计算的是视椎的几何中心，并非精确的最小外接球心；优点是计算非常快，缺点是算出的球不够紧凑，在
  ///          near 端会有较大的冗余。
  static Sphere frustumCircumSphere(const std::array<Vec3f, 8>& corners) noexcept {
    Vec3f center = Vec3f::Zero();
    for(const auto& v: corners)
      center += v;
    center /= 8.0f;
  
    f32 max_dist_squared = 0.0f;
    for(const auto& v: corners)
      max_dist_squared = std::max(max_dist_squared, (v - center).squaredNorm());
  
    return {center, std::sqrt(max_dist_squared)};
  }
  Vec3f center{0};
  f32 radius{0};
};

} // namespace Medium
