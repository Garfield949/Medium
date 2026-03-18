///
/// @file Camera.h
/// @brief 相机类型
///
#pragma once

#include "Base/Types.h"
#include "Scene/Node.h"

namespace Medium {
  
  enum class ProjectionMode {
    Persp, Ortho
  };

  /// @class Camera
  /// @brief 相机类型
  /// @details 管理与构造视图矩阵、投影矩阵相关的变量
  class Camera : public Node{
  public:
    explicit Camera(std::string&& name, ProjectionMode mode)
      : Node(std::move(name)), mode_(mode) {}
    
    void setPerspParams(f32 fov,  f32 near, f32 aspect_ratio)      noexcept;
    void setOrthoParams(f32 near, f32 far,  f32 width, f32 height) noexcept;

    // Getters
    [[nodiscard]] const Mat4f& projMatrix() noexcept;
    [[nodiscard]] Mat4f        viewMatrix() noexcept;
    [[nodiscard]] Mat4f        matrixVP()   noexcept;

  private:
    // 实际维护投影矩阵的方法
    void updateMatProj() noexcept;

    ProjectionMode mode_;

    f32 fov_y_        = 0.0f;
    f32 aspect_ratio_ = 0.0f;
    f32 near_         = 0.0f;

    f32 far_          = 0.0f;
    f32 width_ortho_  = 0.0f;
    f32 height_ortho_ = 0.0f;

    // 投影矩阵
    Mat4f matrix_projection_ = Mat4f::Zero();

    // 脏标记，用于确定是否更新投影矩阵
    bool dirty_proj_matrix_ = true;
  };

} // namespace Medium
