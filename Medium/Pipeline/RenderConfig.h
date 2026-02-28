///
/// @file RenderConfig.h
/// @brief 提供渲染参数管理
///
#pragma once

#include "Base/Color.h"
#include "Base/Types.h"

namespace Medium {

    /// @struct RenderConfig
    /// @brief 渲染参数类型
    /// @details vp 指代 viewport
    struct RenderConfig {
        RenderConfig(u32 w, u32 h) :
            width_vp{w}, height_vp{h}, title{"Medium Canvas"}, clear_color{0} {}

        [[nodiscard]] constexpr f32 vpRatio() const {
            return static_cast<f32>(width_vp) / static_cast<f32>(height_vp);
        }

        u32 width_vp;
        u32 height_vp;
        std::string title;
        Color clear_color;

        // Reversed-Z
        static constexpr f32 MEDIUM_DEPTH_CLEAR_VALUE = 0.0f;
    };

} // namespace Medium