///
/// @file Controller.cpp
/// @brief
///
#include "Controller.h"

#include "Scene/Node.h"
#include "Input.h"

namespace Medium {

    Controller::Controller(Node* attach_to, Input* input) noexcept
        : attach_to_(attach_to), input_(input) {}

    void Controller::update(f32 dt) noexcept {
        assert(attach_to_ && input_);

        // 由于 delta 要与灵敏度等参数相乘，所以转为浮点类型
        Vec2f delta = input_->mouse_move_delta().cast<f32>();

        // 按住鼠标右键才允许视角控制
        if(input_->mouse_btn_held(MOUSE_BTN_2)) {
            if(std::abs(delta.x()) > 0.1) // 0.1 用于过滤鼠标微小的位移
                attach_to_->transform
                    .rotate(Vec3f::UnitY(), delta.x() * mouse_sensitivity_, Space::World);

            if(std::abs(delta.y()) > 0.1)
                attach_to_->transform
                    .rotate(Vec3f::UnitX(), delta.y() * mouse_sensitivity_, Space::Self);
        }

        // 获取相机的物体轴
        Vec3f forward = attach_to_->transform.forward();
        Vec3f right   = attach_to_->transform.right();
        Vec3f up_ref  = Vec3f::UnitY();

        // 位移 = 速度 * 时间
        f32 trans = move_speed_ * dt;

        if (input_->key_held(KB_KEY_W)) {
            attach_to_->transform.translate(forward * trans);
        }
        if (input_->key_held(KB_KEY_S)) {
            attach_to_->transform.translate(-forward * trans);
        }
        if (input_->key_held(KB_KEY_A)) {
            attach_to_->transform.translate(-right * trans);
        }
        if (input_->key_held(KB_KEY_D)) {
            attach_to_->transform.translate(right * trans);
        }
        if (input_->key_held(KB_KEY_SPACE)) {
            attach_to_->transform.translate(up_ref * trans);
        }
        if (input_->key_held(KB_KEY_LEFT_SHIFT)) {
            attach_to_->transform.translate(-up_ref * trans);
        }

    }

} // namespace Medium
