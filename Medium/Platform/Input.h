///
/// @file Input.h
/// @brief 输入处理类型
///
#pragma once

#include <SDL3/SDL.h>

#include "Base/Types.h"

namespace Medium {

class Input {
public:
  Input() = default;

  void update();

  bool key_held(SDL_Scancode key) const noexcept {
    return key_state_ && key_state_[key];
  }

  bool mouse_btn_held(Uint32 button) const noexcept {
    return (mouse_state_ & SDL_BUTTON_MASK(button)) != 0;
  }

  // 获取鼠标从上一 Tick 到这一 Tick 移动的偏移。
  Vec2i mouse_move_delta() const noexcept { return mouse_delta_; }

private:
  SDL_MouseButtonFlags mouse_state_ = 0; // 0 代表没有按鼠标的任何键。
  const bool* key_state_ = nullptr;      // 指向键盘状态数组的指针。

  bool is_first_tick_ = true;

  Vec2i curr_mouse_pos_ = {0, 0};
  Vec2i last_mouse_pos_ = {0, 0};
  Vec2i mouse_delta_    = {0, 0};
};

} // Medium