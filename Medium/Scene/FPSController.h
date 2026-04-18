///
/// @file FPSController.h
/// @brief
///
#pragma once

#include "Base/Types.h"

namespace Medium {

class Node;
class Input;

class FPSController {
public:
  explicit FPSController() = default;

  void bind(Node* host, const Input* input) noexcept {
    assert(host && input);
    host_  = host;
    input_ = input;
  }

  void update(f32 dt) const noexcept;

private:
  f32 move_speed_ = 1.5f;
  f32 mouse_sensitivity_ = 0.2f;

  Node* host_ = nullptr;
  const Input* input_ = nullptr;
};

} // namespace Medium
