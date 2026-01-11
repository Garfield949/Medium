#pragma once

#include "Types.h"

namespace Medium {

    constexpr float PI = EIGEN_PI;
    constexpr float INF_F32 = std::numeric_limits<float>::infinity();

    // 度数转弧度
    constexpr float radians(float deg) {
        return PI * deg / 180.0f;
    }

    // 弧度转度数
    constexpr float degrees(float rad) {
        return rad * 180.0f / PI;
    }

} // namespace Medium
