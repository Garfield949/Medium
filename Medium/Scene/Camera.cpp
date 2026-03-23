///
/// @file Camera.cpp
///
#include "Camera.h"

#include "Base/Math.h"
#include "Base/Types.h"

namespace Medium {

  void Camera::setPerspParams(f32 fov, f32 near, f32 aspect_ratio) noexcept {
    fov_y_             = std::clamp(fov, 1.0f, 179.0f);
    near_              = std::max(near, 0.0001f);
    aspect_ratio_      = aspect_ratio;
    dirty_proj_matrix_ = true;
  }

  void Camera::setOrthoParams(f32 near, f32 far, f32 width, f32 height) noexcept {
    near_              = std::max(near, 0.0001f);
    far_               = far;
    width_ortho_       = width;
    height_ortho_      = height;
    dirty_proj_matrix_ = true;
  }

  // 实现 Camera 成员函数 viewMatrix
  Mat4f Camera::viewMatrix() const noexcept {
    return transform.worldMatrixInverse();
  }

  // 相关参数更新再维护投影矩阵
  const Mat4f& Camera::projMatrix() const noexcept {
    if(dirty_proj_matrix_) {
      updateMatProj();
      dirty_proj_matrix_ = false;
    }
    return matrix_projection_;
  }
  
  Mat4f Camera::vpMatrix() const noexcept {
    return projMatrix() * viewMatrix();
  }

  // 左手系
  void Camera::updateMatProj() const noexcept {
    if(proj_type_ == Projection::Persp) {
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
    else {
      const f32 zoomx = 2.0f / width_ortho_;
      const f32 zoomy = 2.0f / height_ortho_;
      const f32 A =  1.0f / (near_ - far_);
      const f32 B = -far_ / (near_ - far_);
      matrix_projection_.row(0) << zoomx, 0, 0, 0;
      matrix_projection_.row(1) << 0, zoomy, 0, 0;
      matrix_projection_.row(2) << 0, 0,     A, B;
      matrix_projection_.row(3) << 0, 0,     0, 1;
    }
  }

} // namespace Medium