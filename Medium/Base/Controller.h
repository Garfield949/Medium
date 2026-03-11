///
/// @file Controller.h
/// @brief
///
#pragma once

#include "Base/Types.h"

namespace Medium {

  // 前向声明
  class Node;
  class Input;

  class Controller {
  public:
    Controller(Node* attach_to, Input* input) noexcept;

    void update(f32 dt) noexcept;
  private:
    f32 move_speed_ = 1.5f;
    f32 mouse_sensitivity_ = 0.2f;

    Node* attach_to_     = nullptr;
    Input* input_ = nullptr;
  };

} // namespace Medium
