#include "Color.h"

namespace Medium {

    // 默认alpha为255
    Color::Color(u8 r, u8 g, u8 b, u8 a)
        :argb((a << 24) | (r << 16) | (g << 8) | b) {
    }

    // 委托构造：灰度
    Color::Color(u8 greyScale, u8 a)
        :Color(greyScale, greyScale, greyScale, a){
    }

    // 委托构造：默认黑色
    Color::Color()
        :Color(0, 0, 0, 255) {
    }

} // namespace Medium
