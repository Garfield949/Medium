///
/// @file Math.h
/// @brief 放置一些数学常量和方法
///
#pragma once

#include "Base/Types.h"

namespace Medium::Math {

inline constexpr f32 Pi               = 3.141592653f;
inline constexpr f32 KindaSmallNumber = 1e-4f;
inline constexpr f32 VerySmallNumber  = 1e-6f;
inline constexpr f32 KindaLikeOne     = 0.999f;

} // namespace Medium::Math

namespace Medium::Math {

// 判定浮点数是否接近零
// std::abs 到 C++23 才是 constexpr.
[[nodiscard]]
constexpr bool isNearlyZero(f32 value, f32 tolerance = VerySmallNumber) noexcept {
  return std::abs(value) <= tolerance;
}

[[nodiscard]]
constexpr bool isNearlyOne(f32 value, f32 tolerance = KindaLikeOne) noexcept {
  return value >= tolerance;
}

/// @brief 判断 val 是否在给定区间里
template<typename T> [[nodiscard]]
bool isBetween(T val, T min, T max) noexcept {
  return val >= min && val <= max;
}

/// @brief 度数转弧度
[[nodiscard]]
constexpr f32 deg_to_rad(f32 deg) noexcept {
  return Pi * deg / 180.0f;
}

/// @brief 弧度转度数
[[nodiscard]]
constexpr f32 rad_to_deg(f32 rad) noexcept {
  return rad * 180.0f / Pi;
}

/// @brief 浮点向量叉积；
inline f32 cross2D(const Vec2f& v1, const Vec2f& v2) noexcept {
  return v1.x() * v2.y() - v2.x() * v1.y();
}

/// @brief 整型向量叉积；
/// @details  两个 i32 相乘有溢出的风险，返回 i64 更安全。
inline i64 cross2D(const Vec2i& v1, const Vec2i& v2) noexcept {
  return static_cast<i64>(v1.x()) * v2.y() - static_cast<i64>(v2.x()) * v1.y(); // y() 会自动被提升到 i64.
}

/// @brief 线性插值
/// @details 因子 t 取值范围 [0, 1]；
///          想象为一个滑轨，往 0 的方向滑动，就会更多取得 a 的贡献；往 1 的方向滑动，就会更多取得 b 的贡献。
template<typename T> [[nodiscard]]
T lerp(const T& a, const T& b, f32 t) noexcept {
  return a * (1.0f - t) + b * t;
}

/// @brief 欧拉角转四元数
/// @param    euler_deg 欧拉角（度数）Pitch(X), Heading(Y), Bank(Z)
/*  1. AngleAxis 是旋转的一种「中间表达式」，其对乘号做了重载，相乘隐式得到一个 Quaternion.
 *  2. 绕世界固定轴 Z-X-Y 旋转等价于绕对象轴 Y-X-Z 旋转.
 *  3. 复合旋转为右结合：rot * v = Heading * (Pitch * (Bank * v))
 *     固定轴视角为 Bank(Z) -> Pitch(X) -> Heading(Y)，局部轴视角为 Yaw(Y) -> Pitch(X) -> Roll(Z)
 *  4. 该项目采用 FPS 控制，固采用 YXZ 的顺序旋转。                                               */
[[nodiscard]]
inline Quatf eulerToQuatf(const Vec3f& euler_deg) {
  f32 x_rad = deg_to_rad(euler_deg.x());
  f32 y_rad = deg_to_rad(euler_deg.y());
  f32 z_rad = deg_to_rad(euler_deg.z());

  using namespace Eigen;
  Quatf rot = AngleAxisf(y_rad, Vec3f::UnitY())
            * AngleAxisf(x_rad, Vec3f::UnitX())
            * AngleAxisf(z_rad, Vec3f::UnitZ());

  return rot;
}

} // namespace Medium::Math
