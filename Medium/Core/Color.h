///
/// @file Color.h
/// @brief 颜色类型
///
#pragma once

#include "Types.h"

namespace Medium {

    /// @struct Color
    /// @brief 提供对 uint32_t 的包装
    /// @details 内存布局：小端法 Little Endian：B G R A. 各通道占一个 u8
    struct Color {
        // 构造函数，默认 alpha 为 255
        // 采用显式转换，否则值会被隐式提升为 int，符号位为 1 时会变为负数（C++标准中，有符号数左移导致符号位翻转是未定义行为）
        // 其实不转换也没事，因为 argb 是通过位模式与出来的
        constexpr Color(u8 r, u8 g, u8 b, u8 a = 255)
            : data{(static_cast<u32>(a) << 24) | (static_cast<u32>(r) << 16) |
                   (static_cast<u32>(g) << 8)  |  static_cast<u32>(b)} {
        }
        // 委托构造：灰度
        constexpr Color(u8 grey_scale, u8 a = 255)
            : Color{grey_scale, grey_scale, grey_scale, a} {}

        // 委托构造：默认黑色
        constexpr Color() : Color{0} {}

        Color operator*(f32 value) const noexcept{
            if(value <= 0) return Black();
            if(value >= 1) return *this;

            // 定点数映射，将 0.0 ~ 1.0 映射到 0 ~ 256
            u8 scale = static_cast<u8>(value * 256);

            u8 new_r = (r() * scale) >> 8; // 移位代替除法
            u8 new_g = (g() * scale) >> 8;
            u8 new_b = (b() * scale) >> 8;

            return Color{new_r, new_g, new_b, a()};
        }

        // 静态工厂
        static Color Black() noexcept { return {}; }
        static Color White() noexcept { return {255}; }
        static Color Red()   noexcept { return {255, 0, 0}; }
        static Color Green() noexcept { return {0, 255, 0}; }
        static Color Blue()  noexcept { return {0, 0, 255}; }
        static Color Navy()  noexcept { return {79, 147, 184}; }

        // 取通道分量
        [[nodiscard]] constexpr u8 a() const noexcept { return (data >> 24) & 0xFF; }
        [[nodiscard]] constexpr u8 r() const noexcept { return (data >> 16) & 0xFF; }
        [[nodiscard]] constexpr u8 g() const noexcept { return (data >>  8) & 0xFF; }
        [[nodiscard]] constexpr u8 b() const noexcept { return data & 0xFF; }

        // 低层数据
        u32 data;
    };

} // namespace Medium