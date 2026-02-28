///
/// @file   Subpixel.h
/// @author Garfield949
/// @date   2026/2/28 09:35
/// @brief  次像素精度。可这么理解：原本 1 个像素单位是 1cm. 现在引入了更小的度量单位毫米，则
///         原本 10.4cm 的像素可被细分为 104mm.
///
#pragma once

#include "Types.h"

namespace Medium::Subpixel {

    // 定义次像素将一个像素的边划为几格。
    inline constexpr i32 Bits  = 4;

    // 即一个像素共被细分为 2 ^ Bits 格。
    inline constexpr i32 Scale = (1 << Bits);

    // 即 0xF(15)，用来获取浮点坐标提升到定点数后，其小数部分占了次像素的几格；
    // 比如 1.25 * 16 = 20，而 20 & Mask = 0001 0100 & 0000 1111 = 0100 = 4. 其实就是取余，但位运算速度更快。
    inline constexpr i32 Mask  = (1 << Bits) - 1;

    // 定义像素中心偏移为 8 (原像素中心 +0.5f，乘 16 后变成 8)。
    inline constexpr i32 Half  = Scale >> 1;

    // 将浮点坐标提升为定点数，0.5f 的偏置是为了优化四舍五入的行为。
    inline i32 fromFloat(const f32 value) {
        return static_cast<i32>(value * Scale + 0.5f);
    }
    
} // namespace Medium::Subpixel