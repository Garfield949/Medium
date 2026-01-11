#pragma once

#include "Types.h"

namespace Medium {

    struct Color {
        Color();
        Color(u8 r, u8 g, u8 b, u8 a = 255);
        Color(u8 greyScale, u8 a = 255);

        // 静态工厂
        static Color Black() {
            return {};
        }
        static Color White() {
            return {255};
        }

        // 唯一成员变量
        u32 argb;
    };

} // namespace Medium