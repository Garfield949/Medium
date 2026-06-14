///
/// @file FPSController.h
/// @brief
///
#pragma once

#include "Base/Types.h"

namespace Medium {

class Node;
class Input;

/// @class FPSController
/// @brief 第一人称控制器，minecraft 创造模式风格。
class FPSController {
public:
  static constexpr f32 kMaxPitchDeg = 89.0f;
  
  explicit FPSController() = default;

  void bind(Node* host, const Input* input) noexcept;

  void update(f32 dt) noexcept;

private:
  // 世界单位/s
  f32 move_speed_ = 1.5f;
  f32 mouse_sensitivity_ = 0.2f;

  // 由控制器维护相机的绝对旋转状态。
  f32 heading_deg_ = 0.0f;
  f32 pitch_deg_   = 0.0f;

  Node* host_ = nullptr;
  const Input* input_ = nullptr;
};

} // namespace Medium
