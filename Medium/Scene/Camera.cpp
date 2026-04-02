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
    near_    = std::max(near, 0.0001f);
    far_     = far;
    w_ortho_ = width;
    h_ortho_ = height;
    dirty_proj_matrix_ = true;
  }
  
  void Camera::setAspectRatio(f32 aspect_ratio_new) noexcept {
    if (aspect_ratio_new == aspect_ratio_) return;
    aspect_ratio_ = aspect_ratio_new;
    dirty_proj_matrix_ = true;
  }

  // 实现 Camera 成员函数 viewMatrix
  Mat4f Camera::viewMatrix() const noexcept {
    return transform.worldMatrixInverse();
  }

  std::array<Vec3f, 8> Camera::slicedFrustumCorners(f32 slice_far) const noexcept {
    f32 tan_half_fov_y = std::tanf(Math::deg_to_rad(fov_y_ * 0.5f));

    f32 near_y = tan_half_fov_y * near_;
    f32 near_x = near_y * aspect_ratio_ ;

    f32 far_y = tan_half_fov_y * slice_far;
    f32 far_x = far_y * aspect_ratio_;

    // 1. std::array 是一个封装了原生数组的类型。所以第一层括号是初始化一个匿名 std::array
    //    自身，第二层括号才是初始化内部的数组；
    // 2. 第二层括号是将数据指定初始化 std::array 的第一个成员（即唯一的原生数组）；
    // 3. 如果 T 是简单的 POD 类型，则允许类似 std::array<int, 3> a{1, 2, 3} 这样的初始化。
    return std::array<Vec3f, 8>{{
      {-near_x,  near_y, near_}, {near_x,  near_y, near_},
      {-near_x, -near_y, near_}, {near_x, -near_y, near_},

      {-far_x,  far_y, slice_far}, {far_x,  far_y, slice_far},
      {-far_x, -far_y, slice_far}, {far_x, -far_y, slice_far}
    }};
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
    if(mode_ == Mode::Persp) {
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
      // 正交投影同样使用翻转 z: Near -> 1.0f, Far -> 0.0f
      // 目的：管线中深度测试的逻辑是按照翻转 z 设置的，这里是为了配合。正交投影的 Zndc 是线性的，不会出现分布不均。
      const f32 zoomx = 2.0f / w_ortho_;
      const f32 zoomy = 2.0f / h_ortho_;
      const f32 A =  1.0f / (near_ - far_);
      const f32 B = -far_ / (near_ - far_);
      matrix_projection_.row(0) << zoomx, 0, 0, 0;
      matrix_projection_.row(1) << 0, zoomy, 0, 0;
      matrix_projection_.row(2) << 0, 0,     A, B;
      matrix_projection_.row(3) << 0, 0,     0, 1;
    }
  }

} // namespace Medium