///
/// @file   Subpixel.h
/// @author Garfield949
/// @date   2026/2/28 09:35
/// @brief  次像素精度。可这么理解：原本 1 个像素单位是 1cm. 现在引入了更小的度量单位毫米，则
///         原本 10.4cm 的像素可被细分为 104mm
///
#pragma once

#include "Base/Types.h"

namespace Medium::Subpixel {

// 定义次像素边长细分位数。
inline constexpr i32 kBits  = 4;

// 单个像素的边长细分为 2 ^ Bits 格，这里为 2 ^ 4 = 16 格。
inline constexpr i32 kScale = 1 << kBits;

// 定义像素中心偏移为 8 (原像素中心 +0.5f，乘 16 后变成 8)
inline constexpr i32 kHalf  = kScale >> 1;

// 将浮点坐标提升为定点数。
// 1. 未提升之前，浮点坐标的最大舍入误差为 0.5:
//    假设有两个坐标 10 和 10.0625，原本是两个不同的位置，舍入后却都均量化为 10, 精细度不够。
// 2. 当细分边长为 16 时，一个次像素单位占据 1/16 = 0.0625 个像素，此时：
//    - 坐标 10 * 16 = 160; 坐标 10.0625 * 16 = 161. 两个距离极近的坐标在次像素空间可被映射到不同的位置上。
//    - 10.03125 即一个次像素的中间值，10.03125 * 16 = 160.5.
//    - 换句话说，次像素空间的最大舍入误差为 1/16 像素 * 1/2 = 1/32 像素，精细度比原像素空间提升了 16 倍。
inline i32 fromF32(const f32 value) {
  return static_cast<i32>(std::round(value * kScale));
}

} // namespace Medium::Subpixel