#include "Input.h"

namespace Medium {

void Input::update() {

  f32 mouse_x = 0.0f;
  f32 mouse_y = 0.0f;
  mouse_state_ = SDL_GetMouseState(&mouse_x, &mouse_y);
  key_state_   = SDL_GetKeyboardState(nullptr);

  Vec2i mouse_pos{std::lround(mouse_x), std::lround(mouse_y)};

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