///
/// @file Camera.cpp
///
#include "Camera.h"

#include "Core/Math/Math.h"

namespace Medium {

    // 实现 Camera 成员函数 setFOV
    void Camera::setFOV(f32 fov) noexcept {
        fov_y_ = std::clamp(fov, 1.0f, 179.0f);
        dirty_proj_matrix_ = true;
    }

    // 实现 Camera 成员函数 setNearFar
    void Camera::setNear(f32 near_plane) noexcept {
        near_ = near_plane;
        dirty_proj_matrix_ = true;
    }

    // 实现 Camera 成员函数 setAspectRatio
    void Camera::setAspectRatio(f32 aspect_ratio) noexcept {
        aspect_ratio_ = aspect_ratio;
        dirty_proj_matrix_ = true;
    }

    // 实现 Camera 成员函数 viewMatrix
    Mat4f Camera::viewMatrix() noexcept {
        return transform.worldMatrixInverse();
    }

    // 相关参数更新再维护投影矩阵
    const Mat4f& Camera::projMatrix() noexcept {
        if(dirty_proj_matrix_) {
            updateMatProj();
            dirty_proj_matrix_ = false;
        }
        return matrix_projection_;
    }

    // 采用左手系
    void Camera::updateMatProj() noexcept {
        // 计算缩放
        const f32 zoomy = 1.0f / tanf(Math::deg_to_rad(fov_y_ / 2.0f));
        const f32 zoomx = zoomy / aspect_ratio_;
        // 翻转 Z, Reversed-Z
        // Near -> 1.0f, Far -> 0.0f
        // A = -Near / (Far-Near);
        // B = (Near*Far) / (Far-Near);
        // Far 设为了无限远处，故 A 退化为 0，B 退化为 Near
        matrix_projection_.row(0) << zoomx, 0, 0, 0;
        matrix_projection_.row(1) << 0, zoomy, 0, 0;
        matrix_projection_.row(2) << 0, 0, 0, near_;
        matrix_projection_.row(3) << 0, 0, 1,     0;
    }

} // namespace Medium