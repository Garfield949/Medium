///
/// @file Transform.h
/// @brief 变换组件，左手系 + 列向量惯例。
///
#pragma once

#include "Base/Types.h"

namespace Medium {

  class Node;

  enum class Space {
    Self, Parent, World
  };

  /// @class Transform
  /// @details 该类型负责维护所绑定 Node 在每一帧的 TRS.
  class Transform {
    friend class Node;
  public:
    // 默认构造，对象初始为单位（初始）姿态 identity / home orientation.
    explicit Transform(Node* belong_to) noexcept;

    // ---------- Setters: 设置绝对状态
    void setPosition(const Vec3f& pos)       noexcept; // 设置位置
    void setScale(f32 scale)                 noexcept; // 设置均匀缩放
    void setRotation(const Vec3f& euler_deg) noexcept; // 设置旋转，传入欧拉角度数 Pitch, Heading, Bank
    void setRotation(const Quatf& rotation)  noexcept; // 设置旋转，传入四元数
    void lookAt(const Vec3f& target, const Vec3f& up_ref = Vec3f::UnitY()) noexcept; // 给定面向目标，更新绑定对象的旋转姿态

    // ---------- Modifiers: 增量修改
    /// @brief 平移
    /// @param delta            分量 x,y,z 分别对应父空间轴上的位移
    void translate(const Vec3f& delta, Space space = Space::Self) noexcept;

    /// @brief 复合轴旋转
    /// @param euler_degrees    欧拉角度数。分量 x,y,z 对应 Pitch,Yaw/Heading,Roll/Bank.
    /// @param space            指定参考系。Space::Local 为局部系，Space::World 为世界系。
    void rotate(const Vec3f& euler_degrees, Space space = Space::Self) noexcept;

    /// @brief 绕指定轴旋转
    /// @param axis             指定轴。
    /// @param euler_degree     指定旋转度数。
    /// @param space            指定参考系。Space::Local 为局部系，Space::World 为世界系。
    void rotate(const Vec3f& axis, f32 euler_degree, Space space = Space::Self) noexcept;

    // ---------- Getters: 获取状态
    [[nodiscard]] const Vec3f& position() const noexcept { return position_local_; }
    [[nodiscard]] const Quatf& rotation() const noexcept { return rotation_local_; }
    [[nodiscard]] f32          scale()    const noexcept { return scale_local_; }

    [[nodiscard]] Vec3f worldPosition() noexcept;
    [[nodiscard]] Quatf worldRotation() noexcept;

    [[nodiscard]] const Mat4f& worldMatrix() noexcept;
    [[nodiscard]] const Mat4f& localMatrix() noexcept;
    [[nodiscard]] Mat4f worldMatrixInverse() noexcept;

    [[nodiscard]] Vec3f forward() noexcept;
    [[nodiscard]] Vec3f up()      noexcept;
    [[nodiscard]] Vec3f right()   noexcept;

  private:
    void updateDirtyFlags()  noexcept; // 递归地更新附属节点的孩子的脏标记
    void updateLocalMatrix() noexcept; // 实际计算局部矩阵
    void updateWorldMatrix() noexcept; // 实际计算世界矩阵

    Node *belong_to_; // 附属节点指针，用于遍历其子节点

    // 父节点空间下的 TRS 数据
    Vec3f position_local_ = Vec3f::Zero();
    Quatf rotation_local_ = Quatf::Identity();
    f32   scale_local_    = 1.0f;

    // Transform 的矩阵缓存
    Mat4f matrix_local_ = Mat4f::Identity(); // 局部矩阵
    Mat4f matrix_world_ = Mat4f::Identity(); // 世界矩阵

    bool dirty_local_ = true; // 局部矩阵更新脏标记
    bool dirty_world_ = true; // 世界矩阵更新脏标记
  }; // class Transform

} // namespace Medium