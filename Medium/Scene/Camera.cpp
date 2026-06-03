///
/// @file Camera.cpp
///
#include "Camera.h"

#include "Base/Math/Math.h"
#include "Base/Types.h"

namespace Medium {

void Camera::setPerspParams(f32 fov, f32 near, f32 aspect_ratio) noexcept {
  fov_y_             = std::clamp(fov, 1.0f, 179.0f);
  near_              = std::max(near, MinimumNearPlane);
  aspect_ratio_      = aspect_ratio;
  dirty_proj_matrix_ = true;
}

void Camera::setOrthoParams(f32 near, f32 far, f32 width, f32 height) noexcept {
  near_    = std::max(near, MinimumNearPlane);
  far_     = std::max(far, near_ + Math::KINDA_SMALL_NUMBER);
  w_ortho_ = width;
  h_ortho_ = height;
  dirty_proj_matrix_ = true;
}

void Camera::setAspectRatio(f32 aspect_ratio_new) noexcept {
  if (aspect_ratio_new == aspect_ratio_) return;
  aspect_ratio_ = aspect_ratio_new;
  dirty_proj_matrix_ = true;
}

Mat4f Camera::viewMatrix() const noexcept {
  return transform.worldMatrixInverse();
}

// TODO: 目前这个方法仅适用于透视相机
Sphere Camera::slicedFrustumCircumSphereWorld(f32 slice_near, f32 slice_far) const noexcept {
  slice_near = std::max(slice_near, near_);
  slice_far  = std::max(slice_far,  slice_near + Math::KINDA_SMALL_NUMBER);
  
  const Sphere sphere_local = slicedFrustumCircumSphereLocal(slice_near, slice_far);

  const Mat4f& mat_world = transform.worldMatrix();

  const Vec3f center_world = (mat_world * sphere_local.center.homogeneous()).head<3>();

  return {center_world, sphere_local.radius * transform.worldScale()};
}

std::array<Vec3f, 8> Camera::slicedFrustumCornersWorld(f32 slice_near, f32 slice_far) const noexcept {
  slice_near = std::max(slice_near, near_);
  slice_far  = std::max(slice_far,  slice_near + Math::KINDA_SMALL_NUMBER);
  
  const auto corners_local = slicedFrustumCornersLocal(slice_near, slice_far);

  const Mat4f& mat_world = this->transform.worldMatrix();

  std::array<Vec3f, 8> corners_world;
  for(int i = 0; i < 8; i++)
    corners_world[i] = (mat_world * corners_local[i].homogeneous()).head<3>();

  return corners_world;
}

// 相关参数更新再维护投影矩阵
const Mat4f& Camera::projMatrix() const noexcept {
  if(dirty_proj_matrix_) {
    updateMatProj();
    dirty_proj_matrix_ = false;
  }
  return matrix_projection_;
}

Mat4f Camera::viewProjMatrix() const noexcept {
  return projMatrix() * viewMatrix();
}

// 左手系
void Camera::updateMatProj() const noexcept {
  if(mode_ == Mode::Perspective) {
    f32 zoomy = 1.0f / tanf(Math::deg_to_rad(fov_y_ / 2.0f));
    f32 zoomx = zoomy / aspect_ratio_;
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
    f32 zoomx = 2.0f / w_ortho_;
    f32 zoomy = 2.0f / h_ortho_;
    f32 A =  1.0f / (near_ - far_);
    f32 B = -far_ / (near_ - far_);
    matrix_projection_.row(0) << zoomx, 0, 0, 0;
    matrix_projection_.row(1) << 0, zoomy, 0, 0;
    matrix_projection_.row(2) << 0, 0,     A, B;
    matrix_projection_.row(3) << 0, 0,     0, 1;
  }
}

std::array<Vec3f, 8> Camera::slicedFrustumCornersLocal(f32 slice_near, f32 slice_far) const noexcept {
  f32 tan_half_fov_y = std::tanf(Math::deg_to_rad(fov_y_ * 0.5f));

  f32 near_z = slice_near;
  f32 far_z  = slice_far;

  f32 near_y = tan_half_fov_y * near_z;
  f32 near_x = near_y * aspect_ratio_ ;

  f32 far_y = tan_half_fov_y * far_z;
  f32 far_x = far_y * aspect_ratio_;

  // 1. std::array 是一个封装了原生数组的类型。所以第一层括号是初始化一个匿名 std::array
  //    自身，第二层括号才是初始化内部的数组；
  // 2. 第二层括号是将数据指定初始化 std::array 的第一个成员（即唯一的原生数组）；
  // 3. 如果 T 是简单的 POD 类型，则允许类似 std::array<int, 3> a{1, 2, 3} 这样的初始化。
  return std::array<Vec3f, 8>{{
    {-near_x,  near_y, near_z}, {near_x,  near_y, near_z},
    {-near_x, -near_y, near_z}, {near_x, -near_y, near_z},

    {-far_x,  far_y, far_z}, {far_x,  far_y, far_z},
    {-far_x, -far_y, far_z}, {far_x, -far_y, far_z}
  }};
}

// 设 n = near, f = far, a = aspect_ratio.
// 1. 考察 near 面：高度半长 y = n * tan(fov_y/2), 宽度半长 x = y * a. 则关于对角线半长 dn 有
//    (dn)^2 = (1+a^2)(n^2)(tan(fov_y/2))^2. 令 k = (1+a^2)(tan(fov_y/2))^2, 则可推算出：
//    (dn)^2 = k * n^2. 同理对于 far 面有 (df)^2 = k * f^2.
// 2. 根据外接球定义，球心 C 到 8 个角点的半径 R 相等；同时易知球心 C 一定在 Z 轴上，故只需设其 z 值
//    为 z_c. 根据勾股定理，有等式：R^2 = (dn)^2 + (z_c - n)^2 = (df)^2 + (f - z_c)^2.
//    化简得 z_c = (k+1)(f+n)/2, 进一步代入 R^2 即可求出外接球半径。
Sphere Camera::slicedFrustumCircumSphereLocal(f32 slice_near, f32 slice_far) const noexcept {
  f32 n = slice_near, f = slice_far, a = aspect_ratio_;
  f32 t = std::tanf(Math::deg_to_rad(fov_y_ * 0.5f));
  f32 k = (1 + a * a) * t * t;
  // 需考虑一种特殊情况：球心 C 超出了 far 平面。这会导致外接球半径过大，产生不属于视椎体的大量空域，浪费
  // 阴影贴图精度。于是令 z_c <= far, 化简得 k <= (f-n)/(f+n)，得到一个上限值 limit = (f-n)/(f+n).
  // 接下来考虑两种会导致 C 超出 far 的情形：
  // - fov 恒定，far 无限接近 near. 此时 z_c -> (k+1)n, 且 limit -> 0. 而当 fov > 0 时，k 恒大于
  // 0, 即 k 恒大于 limit. 换句话说球心 C 一定超出了 far;
  // - near 和 far 恒定，fov 无限接近 180°. 此时 tan(fov_y/2) -> ∞, 导致球心 C 飞到 +Z 无穷远处。
  // 面对 z_c > f, 即 k > limit 的情况，采取的策略为将 z_c 钳制在 f，即球心在 far 平面中心，确保得到
  // 一个半径最小的合法包围球（不再是外接球，只有 far 的 4 个角点在球面上），以最大限度利用阴影贴图精度。
  f32 limit = (f - n) / (f + n);

  f32 z_c = k <= limit ? (k + 1) * (f + n) * 0.5f : f;

  f32 r_near_squared = n * n * k + (z_c - n) * (z_c - n);
  f32 r_far_squared  = f * f * k + (f - z_c) * (f - z_c);

  // 当出现浮点误差 & 球心超出 far 的情况时，r_near_squared 不再等于 r_far_squared，此时自动取其中最
  // 大者以确保包围球能完美罩住所有角点。
  f32 radius = std::sqrt(std::max(r_near_squared, r_far_squared));

  return {{0, 0, z_c}, radius};
}

} // namespace Medium