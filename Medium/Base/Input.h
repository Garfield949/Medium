///
/// @file Input.h
/// @brief 输入处理类型
///
#pragma once

#include <MiniFB.h>

#include "Base/Types.h"

namespace Medium {

    class Input {
    public:
        Input() = default;

        // 每 Tick 开头调用
        void update(mfb_window * window);

        // 检测是否按下了键盘某个键
        [[nodiscard]] bool key_held(mfb_key key) const noexcept {
            if(key_buffer_ == nullptr) return false;
            return key_buffer_[key] != 0;
        }

        // 检测是否按下了鼠标某个键
        [[nodiscard]] bool mouse_btn_held(mfb_mouse_button button) const noexcept {
            if(mouse_btn_buffer_ == nullptr) return false;
            return mouse_btn_buffer_[button] != 0;
        }

        // 获取鼠标从上一 Tick 到这一 Tick 移动的偏移
        [[nodiscard]] Vec2i mouse_move_delta() const noexcept { return mouse_delta_; }

    private:
        const u8* key_buffer_ = nullptr;
        const u8* mouse_btn_buffer_ = nullptr;

        bool is_first_tick_ = true;

        Vec2i curr_mouse_pos_ = {0, 0};
        Vec2i last_mouse_pos_ = {0, 0};
        Vec2i mouse_delta_    = {0, 0};
    };

} // Medium