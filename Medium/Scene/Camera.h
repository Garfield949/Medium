///
/// @file Camera.h
/// @brief 相机类型
///
#pragma once

#include "Base/Types.h"
#include "Geometry/Frustum.h"
#include "Scene/Node.h"

namespace Medium {

  enum class Projection { Persp, Ortho };

  /// @class Camera
  /// @brief 相机类型
  /// @details 管理与构造视图矩阵、投影矩阵相关的变量
  class Camera : public Node{
  public:
    explicit Camera(std::string name, Projection type)
      : Node{std::move(name)}, proj_type_{type} {}
    
    void setPerspParams(f32 fov,  f32 near, f32 aspect_ratio)      noexcept;
    void setOrthoParams(f32 near, f32 far,  f32 width, f32 height) noexcept;

    Frustum frustum() const noexcept {
      return Frustum::fromMatrixVP(vpMatrix());
    }

    [[nodiscard]] const Mat4f& projMatrix() const noexcept;
    [[nodiscard]] Mat4f viewMatrix() const noexcept;
    [[nodiscard]] Mat4f vpMatrix()   const noexcept;

  private:
    void updateMatProj() const noexcept;

    Projection proj_type_;

    // 透视投影视椎属性
    // 透视投影默认 far 是无限远，near 与正交投影共享
    f32 fov_y_        = 0.0f;
    f32 aspect_ratio_ = 0.0f;
    f32 near_         = 0.0f;

    // 正交投影视椎属性
    f32 far_          = 0.0f;
    f32 width_ortho_  = 0.0f;
    f32 height_ortho_ = 0.0f;

    // 投影矩阵缓存
    mutable Mat4f matrix_projection_ = Mat4f::Zero();
    mutable bool dirty_proj_matrix_ = true;
  };

} // namespace Medium
