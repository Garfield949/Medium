///
/// @file Geometry.h
/// @brief 放置通用的几何、计算相关函数
///
#pragma once

#include "Geometry/Sphere.h"

namespace Medium::Geometry {

/// @brief 屏幕空间重心坐标函数
/// @details    A, B, C 为三角形屏幕坐标，P 为当前要判断的像素点
/// @return     返回 A, B, C 对应重心坐标
inline Vec3f barycentric(const Vec4f &A, const Vec4f &B, const Vec4f &C, f32 px, f32 py) noexcept {
  Vec3f v_x(C.x() - A.x(), B.x() - A.x(), A.x() - px);
  Vec3f v_y(C.y() - A.y(), B.y() - A.y(), A.y() - py);

  // 注意：上方构造 v_x 和 v_y 的顺序决定了 beta 在 u.y，gamma 在 u.x
  Vec3f u = v_x.cross(v_y);

  // 若 u.z 绝对值太小，说明三角形退化，面积为 0
  // u.z 即三角面片有符号面积的二倍，想更激进剔除不可见的微小三角形，这个浮点数判零的阈值还可调得更大（太大会导致丢失很多细微三角形）
  // 返回值带负分量，让后续逻辑剔除
  if (std::abs(u.z()) < Math::KINDA_SMALL_NUMBER) return {-1, 1, 1};

  // 若 P 在三角形外面，则返回值至少有一个分量是负数
  // 返回 alpha, beta, gamma
  return {1.f - (u.x() + u.y()) / u.z(), u.y() / u.z(), u.x() / u.z()};
}

/// @brief 浮点向量叉积；
/// @param v1   向量 1;
/// @param v2   向量 2;
/// @details    1. 计算 v1 x v2. 若 v2 在 v1 的 “右边（顺时针）”，左手系下四指从 v1 弯曲到 v2，则大拇指朝外；
///                而在左手系中，大拇指朝屏幕外说明结果是负值；
///             2. 2D 叉积其实就是 3D 叉积 (x1,y1,0) 与 (x2,y2,0) 叉积所得向量 (0,0,x1y2-x2y1) 的 z 值。
inline f32 cross2D(const Vec2f& v1, const Vec2f& v2) noexcept {
  return v1.x() * v2.y() - v2.x() * v1.y();
}

/// @brief 整型向量叉积；
/// @details    两个 i32 相乘有溢出的风险，返回 i64 更安全。
inline i64 cross2D(const Vec2i& v1, const Vec2i& v2) noexcept {
  return static_cast<i64>(v1.x()) * v2.y() - static_cast<i64>(v2.x()) * v1.y();
}

/// @brief 接收三角面片的三个顶点，计算其在 2D 空间内的朝向；
/// @details    计算向量 (b-a) 与 (c-a) 的叉积；
/// @return     > 0 为 CCW，< 0 为 CW，=0 为共线面积为 0.
inline f32 orient2D(const Vec4f& a, const Vec4f& b, const Vec4f& c) noexcept {
  Vec2f v1 = b.head<2>() - a.head<2>();
  Vec2f v2 = c.head<2>() - a.head<2>();
  return cross2D(v1, v2);
}

/// @brief 边函数计算
inline i64 edgeEquation(const Vec2i& edge_start, const Vec2i& edge_end, const Vec2i& test_point) {
  Vec2i V = test_point - edge_start;
  Vec2i E = edge_end   - edge_start;
  return cross2D(V, E);
}

/// @brief Top-Left Rule 的偏置计算。
/// @param dx    待测试向量的两端 x 增量；
/// @param dy    待测试向量的两端 y 增量；
/// @details     1. 若像素落在两个三角形共享的边上，则让拥有 Top-Left 边的三角形对其着色；
///              2. 顶边的判定：其 y 增量为 0（水平），且方向向右（左手系，顺时针）；
///                 左边的判定：其 y 增量为正。
/// @return 正好落在三角形边上的屏幕坐标的边函数为零，在增量更新的里层判断会通过并被着色。所以判断三条边
///         哪一个不是 Top-Left 型的，返回一个负偏置使该坐标对应边的 Equation 为负，进而无法着色。
inline i32 topLeftBias(i32 dx, i32 dy) noexcept {
  bool is_top_left = (dy == 0 && dx > 0) || dy > 0;
  return is_top_left ? 0 : -1;
};

/// @brief 计算给定视椎的外接球
/// @param corners 接收视椎的角点
/// @return 返回值 Vec2f 的 x 为球心坐标，y 为球半径
/// @details 计算的是视椎的几何中心，并非精确的最小外接球心；优点是计算非常快，缺点是算出的球不够紧凑，在
///          near 端会有较大的冗余。
inline Sphere frustumCircumSphere(const std::array<Vec3f, 8>& corners) noexcept {
  Vec3f center = Vec3f::Zero();
  for(const auto& v: corners)
    center += v;
  center /= 8.0f;

  f32 max_dist_sq = 0.0f;
  for(const auto& v: corners)
    max_dist_sq = std::max(max_dist_sq, (v - center).squaredNorm());

  return {center, std::sqrt(max_dist_sq)};
}

} // namespace Medium
