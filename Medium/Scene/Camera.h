///
/// @file Camera.h
/// @brief 相机类型
///
#pragma once

#include "Base/Types.h"
#include "Scene/Node.h"

namespace Medium {

  /// @class Camera
  /// @brief 相机类型
  /// @details 管理与构造视图矩阵、投影矩阵相关的变量
  class Camera : public Node{
  public:
    explicit Camera(std::string name = "Camera")
      : Node(std::move(name)), fov_y_(90), near_(0.1), aspect_ratio_(1.0) {}

    // Setters
    void setFOV(f32 fov)                  noexcept;
    // 使用 infinite far.
    void setNear(f32 near_plane)          noexcept;
    void setAspectRatio(f32 aspect_ratio) noexcept;

    // Getters
    [[nodiscard]] const Mat4f& projMatrix() noexcept;
    [[nodiscard]] Mat4f        viewMatrix() noexcept;

  private:
    // 实际维护投影矩阵的方法
    void updateMatProj() noexcept;

    // y 轴 FOV、近裁剪面、横纵比
    f32 fov_y_;
    f32 near_;
    f32 aspect_ratio_;

    // 投影矩阵
    Mat4f matrix_projection_ = Mat4f::Zero();

    // 脏标记，用于确定是否更新投影矩阵
    bool dirty_proj_matrix_ = true;
  };

} // namespace Medium
