///
/// @file   Color.cpp
/// @author Garfield949
/// @date   2026/2/24 17:29
/// @brief  
///
#include "Color.h"

namespace Medium {

    Color Color::alphaBlend(const Color& src, const Color& dst) noexcept {
        u8 alpha_src = src.a();

        // Early out, 预检退出。
        // 1. 若当前物体完全不透明，则直接取其颜色；
        // 2. 若当前物体完全透明，则直接取背景颜色。
        if(alpha_src == 0xFF) return src;
        if(alpha_src == 0x00) return dst;

        // 对应 1 - Alpha.
        u8 alpha_dst_weight = 255 - alpha_src;

        // Alpha 原值是 0 ~ 255，表示百分比需映射到 0 ~ 1，所以需除以 255. 这里为了运算快一些用到右移 8 位，
        // 相当于除以 256，差异不大。
        u8 r_new = (src.r() * alpha_src + dst.r() * alpha_dst_weight) >> 8;
        u8 g_new = (src.g() * alpha_src + dst.g() * alpha_dst_weight) >> 8;
        u8 b_new = (src.b() * alpha_src + dst.b() * alpha_dst_weight) >> 8;

        // 混合 Alpha 要理解为物理上的“遮挡覆盖率”。比方说，前景的红色玻璃只让 40% 的光透过；背景虽然有 50% 的
        // 遮挡能力，但也只能在前景透过的那 40% 里发挥作用，也即背景实际贡献的有效遮挡率为 0.4 * 0.5 = 0.2.
        // 最终 Alpha = 前景被遮挡部分 + 透过前景但被背景遮挡部分。
        u8 a_new = alpha_src + ((alpha_dst_weight * dst.a()) >> 8);

        return {r_new, g_new, b_new, a_new};
    }

} // namespace Medium
