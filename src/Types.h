#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>

namespace Medium {

    // 类型别名
    // Affine3f底层是四维矩阵，包含了常用变换方法
    using Aff3f = Eigen::Affine3f;
    using Vec3f = Eigen::Vector3f;
    using Vec4f = Eigen::Vector4f;
    using Vec3i = Eigen::Vector3i;
    using Mat3f = Eigen::Matrix3f;
    using Mat4f = Eigen::Matrix4f;

    // 类型别名
    // u32为ARGB，u8为单通道分量
    using u32 = uint32_t;
    using u8  = uint8_t;

    // 类型别名
    using f32 = float;
    using f64 = double;

} // namespace Medium