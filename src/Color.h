//
// 内存布局 - 小端法 Little Endian：B G R A
//
#pragma once

#include <iostream>

#include "Types.h"

namespace Medium {

    struct Color {
        // 构造函数，默认 alpha 为 255
        // 采用显式转换，否则值会被隐式提升为 int，符号位为 1 时会变为负数（C++标准中，有符号数左移导致符号位翻转是未定义行为）
        // 其实不转换也没事，因为 argb 是通过位模式与出来的
        constexpr Color(u8 r, u8 g, u8 b, u8 a = 255)
            : data{(static_cast<u32>(a) << 24) | (static_cast<u32>(r) << 16) |
                   (static_cast<u32>(g) << 8)  |  static_cast<u32>(b)} {
        }
        // 委托构造：灰度
        constexpr Color(u8 grey, u8 a = 255) : Color{grey, grey, grey, a} {}
        // 委托构造：默认黑色
        constexpr Color() : Color{0} {}

        // 静态工厂
        static Color Black() noexcept { return {}; }
        static Color White() noexcept { return {255}; }
        static Color Red()   noexcept { return {255, 0, 0}; }
        static Color Green() noexcept { return {0, 255, 0}; }
        static Color Blue()  noexcept { return {0, 0, 255}; }

        // 取通道分量
        [[nodiscard]] constexpr u8 a() const noexcept { return (data >> 24) & 0xFF; }
        [[nodiscard]] constexpr u8 r() const noexcept { return (data >> 16) & 0xFF; }
        [[nodiscard]] constexpr u8 g() const noexcept { return (data >>  8) & 0xFF; }
        [[nodiscard]] constexpr u8 b() const noexcept { return data & 0xFF; }

        // 低层数据
        u32 data;
    };

} // namespace Medium