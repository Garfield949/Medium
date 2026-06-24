///
/// @file   Interpolation.h
/// @author Garfield949
/// @date   2026/6/19 10:18
/// @brief  
///
#pragma once

#include "Base/Math/Math.h"

namespace Medium::Interp {

/// @brief 普通重心坐标线性插值；
/// @param attrib0, attrib1, attrib2 三个待混合顶点属性；
/// @param weights 重心坐标 (alpha, beta, gamma)；
/// @return 混合结果。
template <typename T> [[nodiscard]]
T interpolate(const T& attrib0, const T& attrib1, const T& attrib2, const Vec3f& weights) noexcept {
  return attrib0 * weights.x() + attrib1 * weights.y() + attrib2 * weights.z();
}

/// @brief 重心坐标透视矫正插值；
/// @param attrib0, attrib1, attrib2 三个待混合顶点属性；
/// @param weights 重心坐标 (alpha, beta, gamma)；
/// @param w_invs 三个顶点的 1/w 值；
/// @note 要求类型 T 必须支持 * 与 + 运算符重载；
/// @details 1/w 与 attrib/w 在屏幕空间均为线性的。要得到当前像素的 attrib_interp，只需对三个顶点的 attrib/w 进行
/// 插值，再除以当前像素的 1/w（也为插值得到）即得 attrib_interp;
/// @return 混合结果。
template <typename T> [[nodiscard]]
T interpolate(const T& attrib0, const T& attrib1, const T& attrib2, const Vec3f& weights, const Vec3f& w_invs) noexcept {
  // 中间值，用于避免冗余乘法
  f32 tmp0 = w_invs[0] * weights[0];
  f32 tmp1 = w_invs[1] * weights[1];
  f32 tmp2 = w_invs[2] * weights[2];
  // 插值当前像素的 1/w，即透视矫正公式的分母
  f32 w_inv_interp = tmp0 + tmp1 + tmp2;
  // 避免除零，w_inv_interp 极小说明像素位于"无穷远"
  if(std::abs(w_inv_interp) < Math::KINDA_SMALL_NUMBER) return attrib0;
  // 透视矫正公式的分子，对三个顶点的 attrib/w 进行插值
  T val = (attrib0 * tmp0) + (attrib1 * tmp1) + (attrib2 * tmp2);
  // 当前像素的 w，透视矫正公式分母的倒数
  f32 w_interp = 1.0f / w_inv_interp;
  // 除以 1/w 等价于乘以 w
  return val * w_interp;
}

struct PerspWeights {
  f32 w0, w1, w2;
  f32 w_interp;
  static PerspWeights Get(const Vec3f& weights, const Vec3f& w_invs) noexcept {
    f32 tmp0 = w_invs[0] * weights[0];
    f32 tmp1 = w_invs[1] * weights[1];
    f32 tmp2 = w_invs[2] * weights[2];
    f32 w_inv_interp = tmp0 + tmp1 + tmp2;
    f32 w_interp = std::abs(w_inv_interp) < Math::KINDA_SMALL_NUMBER ? -1.0f : 1.0f / w_inv_interp;
    return {tmp0, tmp1, tmp2, w_interp};
  }
};

/// @brief 重心坐标透视矫正插值；
/// @details 预计算权重版本，在 frag 层面只计算一次中间变量。
template <typename T> [[nodiscard]]
T interpolate(const T& attrib0, const T& attrib1, const T& attrib2, const PerspWeights& weights) noexcept {
  T val = (attrib0 * weights.w0) + (attrib1 * weights.w1) + (attrib2 * weights.w2);
  return val * weights.w_interp;
}

} // namespace Medium
