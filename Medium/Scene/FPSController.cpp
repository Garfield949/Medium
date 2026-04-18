///
/// @file FPSController.cpp
/// @brief
///
#include "FPSController.h"

#include "Platform/Input.h"
#include "Scene/Node.h"

namespace Medium {

void FPSController::update(f32 dt) const noexcept {
  // 由于 delta 要与灵敏度等参数相乘，所以转为浮点类型
  Vec2f delta;

  Transform& host_tf = host_->transform;

  // 按住鼠标右键才允许视角控制
  if(input_->mouse_btn_held(MFB_MOUSE_BTN_2)) {
    delta = input_->mouse_move_delta().cast<f32>();
    host_tf.rotate(Vec3f::UnitY(), delta.x() * mouse_sensitivity_, Space::World);
    host_tf.rotate(Vec3f::UnitX(), delta.y() * mouse_sensitivity_, Space::Self);
  }

  // 位移 = 速度 * 时间
  f32 S = move_speed_ * dt;

  if (input_->key_held(MFB_KB_KEY_W)) {
    host_tf.translate( Vec3f::UnitZ() * S, Space::Self);
  }
  if (input_->key_held(MFB_KB_KEY_S)) {
    host_tf.translate(-Vec3f::UnitZ() * S, Space::Self);
  }
  if (input_->key_held(MFB_KB_KEY_A)) {
    host_tf.translate(-Vec3f::UnitX() * S, Space::Self);
  }
  if (input_->key_held(MFB_KB_KEY_D)) {
    host_tf.translate( Vec3f::UnitX() * S, Space::Self);
  }
  if (input_->key_held(MFB_KB_KEY_SPACE)) {
    host_tf.translate( Vec3f::UnitY() * S, Space::World);
  }
  if (input_->key_held(MFB_KB_KEY_LEFT_SHIFT)) {
    host_tf.translate(-Vec3f::UnitY() * S, Space::World);
  }

}

} // namespace Medium
