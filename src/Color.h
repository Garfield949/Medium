#pragma once

#include "Types.h"

namespace Medium {

    struct Color {
        // 构造函数，默认 alpha 为 255
        // 采用显式转换，否则 255 的值会被隐式提升为 int 变为负数
        // 其实不转换也没事，因为 argb 是通过位模式与出来的
        constexpr Color(u8 r, u8 g, u8 b, u8 a)
            : argb((static_cast<u32>(a) << 24) | (static_cast<u32>(r) << 16) |
                   (static_cast<u32>(g) << 8)  |  static_cast<u32>(b)) {
        }

        // 委托构造：灰度
        constexpr Color(u8 greyScale, u8 a = 255)
            :Color(greyScale, greyScale, greyScale, a){
        }

        // 委托构造：默认黑色
        constexpr Color():Color(0, 0, 0, 255) {
        }

        // 静态工厂
        static Color Black() noexcept {
            return {};
        }
        static Color White() noexcept {
            return {255};
        }

        // 唯一成员变量
        u32 argb;
    };

} // namespace Medium