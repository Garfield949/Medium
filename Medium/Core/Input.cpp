#include "Input.h"

namespace Medium {

    void Input::update(mfb_window * window) {
        // 获取当前 Tick 的键盘按键和鼠标按键状态缓冲
        key_buffer_ = mfb_get_key_buffer(window);
        mouse_btn_buffer_ = mfb_get_mouse_button_buffer(window);

        Vec2i mouse_pos{mfb_get_mouse_x(window), mfb_get_mouse_y(window)};

        // 防止第一帧鼠标跳变
        if(is_first_tick_) {
            curr_mouse_pos_ = last_mouse_pos_ = mouse_pos;
            is_first_tick_ = false;
        } else {
            last_mouse_pos_ = curr_mouse_pos_;
            curr_mouse_pos_ = mouse_pos;
        }

        mouse_delta_ = curr_mouse_pos_ - last_mouse_pos_;
    }

} // namespace Medium