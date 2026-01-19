#pragma once

#include "Color.h"
#include "Types.h"

namespace Medium {

    // vp 指代 viewport
    struct RenderConfig {
        RenderConfig(u32 w, u32 h) :
            vp_width{w}, vp_height{h}, title{"Medium Canvas"}, clear_color{0} {}

        u32 vp_width;
        u32 vp_height;
        std::string title;
        Color clear_color;

        [[nodiscard]] f32 vpRatio() const {
            return static_cast<f32>(vp_width) / static_cast<f32>(vp_height);
        }
    };

} // namespace Medium