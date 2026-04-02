///
/// @file Camera.h
/// @brief 相机类型
///
#pragma once

#include "Base/Types.h"
#include "Geometry/Frustum.h"
#include "Scene/Node.h"

namespace Medium {

  /// @class Camera
  /// @brief 相机类型
  /// @details 管理与构造视图矩阵、投影矩阵相关的变量
  class Camera : public Node{
  public:
    static constexpr NodeType Type = NodeType::Camera;

    enum class Mode { 
      Persp, Ortho 
    };

    explicit Camera(std::string name, Mode mode)
      : Node{std::move(name), Type}, mode_{mode} {}

    /// @brief 当构造为一个透视相机时，传入相应参数
    void setPerspParams(f32 fov, f32 near, f32 aspect_ratio) noexcept;
    
    /// @brief 当构造为一个正交相机时，传入相应参数
    void setOrthoParams(f32 near, f32 far, f32 width, f32 height) noexcept;
    
    /// @brief 修改视区宽高比，通常用于窗口缩放
    void setAspectRatio(f32 aspect_ratio) noexcept;
    
    /// @brief 返回相机所属的视椎体，由相机的 view_projection 矩阵构建
    [[nodiscard]] Frustum frustum() const noexcept {
      return Frustum::fromMatrixVP(vpMatrix());
    }
    
    /// @brief 返回相机视椎切片的八个局部空间角点。
    /// @details    由于采用无限远平面，所以返回一个视椎切片：即接收一个 slice_far 作为切片的远平面，near
    ///             依然采用初始化相机时传入的值；先算出 near 和 slice_far 的右上角点，再一一镜像即可得到
    ///             所有 8 个角点。
    [[nodiscard]] std::array<Vec3f, 8> slicedFrustumCorners(f32 slice_far) const noexcept;

    [[nodiscard]] const Mat4f& projMatrix() const noexcept;
    [[nodiscard]] Mat4f viewMatrix()        const noexcept;
    [[nodiscard]] Mat4f vpMatrix()          const noexcept;

  private:
    void updateMatProj() const noexcept;

    Mode mode_;

    // 透视投影视椎属性
    // 透视投影默认 far 是无限远，near 与正交投影共享
    f32 fov_y_        = 0.0f;
    f32 aspect_ratio_ = 0.0f;
    f32 near_         = 0.0f;

    // 正交投影视椎属性
    f32 far_     = 0.0f;
    f32 w_ortho_ = 0.0f;
    f32 h_ortho_ = 0.0f;

    // 投影矩阵缓存
    mutable Mat4f matrix_projection_ = Mat4f::Zero();
    mutable bool  dirty_proj_matrix_ = true;
  };

} // namespace Medium
