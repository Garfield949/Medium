///
/// @file FPSController.cpp
/// @brief
///
#include "FPSController.h"

#include "Base/Math/Math.h"
#include "Platform/Input.h"
#include "Scene/Node.h"

namespace Medium {

void FPSController::bind(Node *host, const Input *input) noexcept {
  assert(host && input);
  host_  = host;
  input_ = input;
  bindNewHost(host);
}

void FPSController::bindNewHost(Node *host) noexcept {
  assert(host);
  host_ = host;
  // 场景可能已经定义了相机的旋转，所以从其 Transform 中反解出当前状态的 heading 和 pitch，否则第一次 update 鼠标
  // 移动时会直接将旋转置为 {0,0,0}.
  Vec3f forward = host_->transform.forward();
  // sin(theta) = y/1 -> theta = asin(y).
  // 注意，当 y 是个负值（左手系里表示向下 pitch），asin(y) 会返回一个负角度，需要取反。
  pitch_deg_ = -Math::rad_to_deg(std::asin(forward.y()));
  // atan2(a,b) = arctan(a/b) -> tan(theta) = x/z -> theta = atan2(x, z).
  heading_deg_ = Math::rad_to_deg(std::atan2(forward.x(), forward.z()));
}

void FPSController::update(f32 dt) noexcept {
  Transform& host_tf = host_->transform;

  // 按住鼠标右键才允许视角控制
  if(input_->mouse_btn_held(SDL_BUTTON_RIGHT)) {
    // 由于 delta 要与灵敏度等参数相乘，所以转为浮点类型
    Vec2f delta = input_->mouse_move_delta().cast<f32>();

    heading_deg_ += delta.x() * mouse_speed_;
    pitch_deg_   += delta.y() * mouse_speed_;
    pitch_deg_ = std::clamp(pitch_deg_, -kMaxPitchDeg, kMaxPitchDeg);
    
    host_tf.setRotation({pitch_deg_, heading_deg_, 0.0f});
  }

  // 位移 = 速度 * 时间
  f32 S = move_speed_ * dt;

  // 取消掉朝向的 y 分量，实现 xz 平面平移行为。
  Vec3f forward = host_tf.forward();
  forward.y() = 0.0f;
  forward.normalize();
  
  Vec3f right = host_tf.right();

  if(input_->key_held(SDL_SCANCODE_W)) {
    host_tf.translate(forward * S, Space::World);
  }
  if(input_->key_held(SDL_SCANCODE_S)) {
    host_tf.translate(-forward * S, Space::World);
  }
  if(input_->key_held(SDL_SCANCODE_A)) {
    host_tf.translate(-right * S, Space::World);
  }
  if(input_->key_held(SDL_SCANCODE_D)) {
    host_tf.translate(right * S, Space::World);
  }
  if(input_->key_held(SDL_SCANCODE_SPACE)) {
    host_tf.translate(Vec3f::UnitY() * S, Space::World);
  }
  if(input_->key_held(SDL_SCANCODE_LSHIFT)) {
    host_tf.translate(-Vec3f::UnitY() * S, Space::World);
  }

}

} // namespace Medium
