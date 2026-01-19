#pragma once

#include "Types.h"

namespace Medium {

    class Camera {
    public:
        Camera()
            : pos_(Vec3f::Zero()), target_(Vec3f::UnitZ()), up_(Vec3f::UnitY()),
              fov_y_(90), near_(0.1), far_(100), aspect_ratio_(1.0) {}

        void move(const Vec3f &delta) noexcept;

        void updateMatView() noexcept;
        void updateMatProj() noexcept;

        // Setters
        void setPos(const Vec3f &position)             noexcept;
        void setTarget(const Vec3f &target)            noexcept;
        void setUp(const Vec3f &up)                    noexcept;
        void setFOV(f32 fov)                           noexcept;
        void setNearFar(f32 near_plane, f32 far_plane) noexcept;
        void setAspectRatio(f32 aspect_ratio)          noexcept;

        // Getters
        [[nodiscard]] const Mat4f& matView() noexcept;
        [[nodiscard]] const Mat4f& matProj() noexcept;

    private:
        Vec3f pos_;
        Vec3f target_;
        Vec3f up_;

        f32 fov_y_;
        f32 near_;
        f32 far_;
        f32 aspect_ratio_;

        Mat4f mat_view_;
        Mat4f mat_proj_;

        bool dirty_view_mat_ = true;
        bool dirty_proj_mat_ = true;
    };

    // 实现 Camera 成员函数 move
    inline void Camera::move(const Vec3f &delta) noexcept {
        pos_ += delta;
        dirty_view_mat_ = true;
    }

    // 实现 Camera 成员函数 setPos
    inline void Camera::setPos(const Vec3f &position) noexcept {
        pos_ = position;
        dirty_view_mat_ = true;
    }

    // 实现 Camera 成员函数 setTarget
    inline void Camera::setTarget(const Vec3f &target) noexcept {
        target_ = target;
        dirty_view_mat_ = true;
    }

    // 实现 Camera 成员函数 setUp
    inline void Camera::setUp(const Vec3f &up) noexcept {
        up_ = up;
        dirty_view_mat_ = true;
    }

    // 实现 Camera 成员函数 setFOV
    inline void Camera::setFOV(f32 fov) noexcept {
        fov_y_ = std::max(1.0f, std::min(fov, 179.0f));
        dirty_proj_mat_ = true;
    }

    // 实现 Camera 成员函数 setNearFar
    inline void Camera::setNearFar(f32 near_plane, f32 far_plane) noexcept {
        assert(near_plane >= 0.1f && "近裁剪面至少为0.1!");
        assert(far_plane > near_plane && "远裁剪面必须大于近裁剪面!");

        near_ = std::max(near_plane, 0.1f);
        far_  = std::max(far_plane, near_ + 0.1f);

        dirty_proj_mat_ = true;
    }

    // 实现 Camera 成员函数 setAspectRatio
    inline void Camera::setAspectRatio(f32 aspect_ratio) noexcept {
        aspect_ratio_ = aspect_ratio;
        dirty_proj_mat_ = true;
    }

} // namespace Medium
