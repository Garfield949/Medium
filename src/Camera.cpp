#include "Camera.h"
#include "Math.h"

namespace Medium {

    void Camera::updateMatView() noexcept {
        // 构建相机坐标系
        // TODO: 万向节死锁
        // TODO: 上层接口使用欧拉角
        Vec3f forward = (target_ - pos_).normalized();
        Vec3f right = up_.cross(forward).normalized();
        Vec3f up_real = forward.cross(right);

        // 要将世界系转换到相机空间，则采用由相机空间的基构成矩阵的逆变换，即转置
        Mat4f rotate = Mat4f::Identity();
        rotate.col(0).head(3) = right;
        rotate.col(1).head(3) = up_real;
        rotate.col(2).head(3) = forward;
        rotate.transposeInPlace();

        Mat4f translate = Mat4f::Identity();
        translate.topRightCorner(3, 1) = -pos_;

        mat_view_ = rotate * translate;
    }

    void Camera::updateMatProj() noexcept {
        const f32 zoomy = 1 / tanf(radians(fov_y_ / 2));
        const f32 zoomx = zoomy / aspect_ratio_;

        Mat4f tmp = Mat4f::Zero();
        tmp <<
            zoomx, 0, 0, 0,
            0, zoomy, 0, 0,
            0, 0, (far_+near_)/(far_-near_), (-2*near_*far_) / (far_-near_),
            0, 0, 1, 0;

        mat_proj_ = tmp;
    }

    const Mat4f& Camera::matView() noexcept {
        if(dirty_view_mat_) {
            updateMatView();
            dirty_view_mat_ = false;
        }
        return mat_view_;
    }

    const Mat4f& Camera::matProj() noexcept {
        if(dirty_proj_mat_) {
            updateMatProj();
            dirty_proj_mat_ = false;
        }
        return mat_proj_;
    }

} // namespace Medium