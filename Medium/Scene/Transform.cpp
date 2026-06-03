///
/// @file   Transform.cpp
/// @author Garfield949
/// @date   2026/1/20 12:22
/// @brief  变换组件，左手系 + 列向量惯例。
///
#include "Transform.h"

#include "Base/Math/Math.h"
#include "Base/Math/RotationMatrix.h"
#include "Scene/Node.h"

namespace Medium {

Transform::Transform(Node* belong_to) noexcept : belong_to_(belong_to) {}

void Transform::setPosition(const Vec3f& pos) noexcept {
  pos_local_ = pos;
  updateDirtyFlags();
}

void Transform::setScale(f32 scale) noexcept {
  scale_local_ = scale;
  updateDirtyFlags();
}

void Transform::setRotation(const Vec3f& euler_deg) noexcept {
  rot_local_ = Math::eulerToQuatf(euler_deg);
  rot_local_.normalize();
  updateDirtyFlags();
}

// 由于 target 是世界坐标，故先算出所绑定 Node 在世界空间的姿态，再转为局部旋转，最后保存为底层四元数。
// 注意数学奇点（非万向节死锁）：1. forward 为零时等价于看向自己，makeFromForward 会返回一个单位矩阵；
//                          2. 当 forward 与 up_ref 重合时等价于垂直向上看，right 叉积结果将为 (0,0,0)，归一化会发生除零。
void Transform::lookAt(const Vec3f& target_world, const Vec3f& up_ref) noexcept {
  Vec3f forward_world = target_world - worldPosition();

  Mat3f rot_world = RotationMatrix::makeFromForward(forward_world, up_ref);

  Quatf R_world{rot_world};

  // 将世界姿态转为局部姿态
  // R_world = parent_R_world * R_local -> R_local = parent_R_world_inv * R_world
  if(const auto ptr = belong_to_->parent_) {
    rot_local_ = ptr->transform.worldRotation().inverse() * R_world;
  } else {
    rot_local_ = R_world;
  }

  updateDirtyFlags();
}

void Transform::translate(const Vec3f& delta, Space space) noexcept {
  if(space == Space::Self) {
    pos_local_ += rot_local_ * delta;
  } else if(const auto ptr = belong_to_->parent_) {
    pos_local_ +=
      ptr->transform.worldMatrixInverse().topLeftCorner<3, 3>() * delta;
  } else {
    pos_local_ += delta;
  }
  updateDirtyFlags();
}

void Transform::rotate(const Vec3f& euler_degrees, Space space) noexcept {
  Quatf delta = Math::eulerToQuatf(euler_degrees);

  rot_local_ = (space == Space::Self) ? (rot_local_ * delta) : (delta * rot_local_);
  rot_local_.normalize();

  updateDirtyFlags();
}

void Transform::rotate(const Vec3f& axis, f32 euler_degree, Space space) noexcept {
  Quatf delta{Eigen::AngleAxisf(Math::deg_to_rad(euler_degree), axis)};

  rot_local_ = (space == Space::Self) ? (rot_local_ * delta) : (delta * rot_local_);
  rot_local_.normalize();

  updateDirtyFlags();
}

const Mat4f& Transform::localMatrix() const noexcept {
  if(dirty_local_) {
    updateLocalMatrix();
    dirty_local_ = false;
  }
  return matrix_local_;
}

Vec3f Transform::worldPosition() const noexcept {
  return worldMatrix().topRightCorner<3, 1>();
}

Quatf Transform::worldRotation() const noexcept {
  if(const auto ptr = belong_to_->parent_)
    return ptr->transform.worldRotation() * rot_local_;
  return rot_local_;
}

f32 Transform::worldScale() const noexcept {
  if(const auto ptr = belong_to_->parent_)
    return ptr->transform.worldScale() * scale_local_;
  return scale_local_;
}

const Mat4f& Transform::worldMatrix() const noexcept {
  if(dirty_world_) {
    updateWorldMatrix();
    dirty_world_ = false;
  }
  return matrix_world_;
}

Mat4f Transform::worldMatrixInverse() const noexcept {
  const auto& mat_world = worldMatrix();
  // ---------- (TRS)^-1 = S^-1 R^-1 T^-1
  // 由于只支持均匀缩放，故只需算某个轴的模长即可得到缩放。
  f32 S = mat_world.col(0).head<3>().norm();
  // 缩放的逆 (S^-1)
  // 即缩放的倒数。若缩放值接近于零则直接赋值为零，避免除零。
  f32 S_inv = Math::isNearlyZero(S) ? 0.0f : 1.0f / S;

  // 旋转的逆 (R^-1 = R^T)
  // 转置后，其行分别为物体当前姿态的物体 x, y, z 轴。
  Mat3f R = mat_world.topLeftCorner<3, 3>() / S;
  Mat3f R_inv = R.transpose();

  // 组合：(S^-1)(R^-1)，将逆缩放值乘到对应的各物体轴上。
  Mat3f RS_inv = R_inv * S_inv;

  // 平移的逆 (T^-1) = (S^-1)(R^-1)(-T)
  Vec3f T = mat_world.topRightCorner<3, 1>();
  Vec3f T_inv = RS_inv * (-T);

  Mat4f matrix_world_inv = Mat4f::Identity();
  matrix_world_inv.topLeftCorner<3, 3>() = RS_inv;
  matrix_world_inv.topRightCorner<3, 1>() = T_inv;
  return matrix_world_inv;
}

Vec3f Transform::forward() const noexcept {
  return worldMatrix().col(2).head<3>().normalized();
}

Vec3f Transform::up() const noexcept {
  return worldMatrix().col(1).head<3>().normalized();
}

Vec3f Transform::right() const noexcept {
  return worldMatrix().col(0).head<3>().normalized();
}

void Transform::updateDirtyFlags() noexcept {
  dirty_local_ = true;
  if(!dirty_world_) { // 若已经是脏的，那么子孙结点在之前肯定也已被赋予脏位了，递归剪枝
    dirty_world_ = true;
    for(auto& child: belong_to_->children_) // 牵一发动全身
      child->transform.updateDirtyFlags();
  }
}

// 填充左上角 3 x 3 旋转 + 缩放部分、右上角平移部分
void Transform::updateLocalMatrix() const noexcept {
  Mat3f R = rot_local_.toRotationMatrix();
  matrix_local_.setIdentity();
  matrix_local_.col(0).head<3>() = R.col(0) * scale_local_;
  matrix_local_.col(1).head<3>() = R.col(1) * scale_local_;
  matrix_local_.col(2).head<3>() = R.col(2) * scale_local_;
  matrix_local_.topRightCorner<3, 1>() = pos_local_;
}

void Transform::updateWorldMatrix() const noexcept {
  if(const auto ptr = belong_to_->parent_)
    matrix_world_ = ptr->transform.worldMatrix() * localMatrix();
  else
    matrix_world_ = localMatrix();
}

} // namespace Medium
