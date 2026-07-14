///
/// @file Camera.h
/// @brief 相机类型
///
#pragma once

#include "Base/Types.h"
#include "Base/Geometry/Frustum.h"
#include "Base/Geometry/Sphere.h"
#include "Scene/Node.h"

namespace Medium {

/// @class Camera
/// @brief 相机类型
/// @details 管理与构造视图矩阵、投影矩阵相关的变量
class Camera : public Node {
public:
  static constexpr NodeType Type = NodeType::Camera;

  // near 固定为 0.1. 其实正交投影可直接将 near 设为 0，因为 “透视除法” 阶段顶点的 w 永远是 1，
  // 不会发生除零，这里是为了兼容透视投影。
  static constexpr f32 kNearPlane = 0.1f;

  enum class Mode { Perspective, Orthographic };

  explicit Camera(std::string name, Mode mode)
    : Node{std::move(name), Type}, mode_{mode} {}

  /// @brief 当构造为一个透视相机时，传入相应参数
  void setPerspParams(f32 fov, f32 aspect_ratio) noexcept;
  
  /// @brief 当构造为一个正交相机时，传入相应参数
  void setOrthoParams(f32 far, f32 width, f32 height) noexcept;
  
  /// @brief 修改视区宽高比，通常用于窗口缩放
  void setAspectRatio(f32 aspect_ratio) noexcept;

  /// @brief 修改 field of view.
  void setFOV(f32 fov) noexcept;
  
  /// @brief 返回相机所属的视椎体，由相机的 view_projection 矩阵构建
  Frustum frustum() const noexcept {
    return Frustum::FromMatrixVP(viewProjMatrix());
  }

  /// @brief 构造相机视椎切片的外接球，球心转换到世界坐标，并根据变换缩放半径。
  Sphere slicedFrustumCircumSphereWorld(f32 slice_near, f32 slice_far) const noexcept;

  /// @brief 返回相机视椎切片的八个世界空间角点。
  /// @details 由于采用无限远平面，所以返回一个视椎切片：slice_near 和 slice_far 作为切片的近平面和远平面，
  std::array<Vec3f, 8> slicedFrustumCornersWorld(f32 slice_near, f32 slice_far) const noexcept;
  
  const Mat4f& projMatrix() const noexcept;
  Mat4f viewMatrix()        const noexcept;
  Mat4f viewProjMatrix()    const noexcept;

  f32 fov() noexcept { return fov_y_; }
  
  /// @brief 返回远平面，对于透视投影返回无穷大。
  f32 far()  const noexcept { 
    return mode_ == Mode::Orthographic ? far_ : std::numeric_limits<f32>::infinity();
  }

private:
  /// @brief 返回相机视椎切片的八个局部空间角点。
  /// @details 先算出 slice_near 和 slice_far 的右上角点，再一一镜像即可得到所有 8 个角点。
  std::array<Vec3f, 8> slicedFrustumCornersLocal(f32 slice_near, f32 slice_far) const noexcept;

  /// @brief 构造相机局部空间视椎外接球。
  /// @details 直接由解析解构造，速度较快。
  Sphere slicedFrustumCircumSphereLocal(f32 slice_near, f32 slice_far) const noexcept;
  
  void updateMatProj() const noexcept;

  Mode mode_;

  // 透视投影视椎属性
  // 透视投影默认 far 是无限远.
  f32 fov_y_        = 0.0f;
  f32 aspect_ratio_ = 0.0f;

  // 正交投影视椎属性
  f32 far_     = 0.0f;
  f32 w_ortho_ = 0.0f;
  f32 h_ortho_ = 0.0f;

  // 投影矩阵缓存
  mutable Mat4f matrix_projection_ = Mat4f::Zero();
  mutable bool  dirty_proj_matrix_ = true;
};

} // namespace Medium
