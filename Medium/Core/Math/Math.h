///
/// @file Math.h
/// @brief 放置一些数学常量和方法
///
#pragma once

#include "Core/Types.h"

namespace Medium::Math {

    inline constexpr f32 MEDIUM_PI = 3.141592653f;
    inline constexpr f32 KINDA_SMALL_NUMBER = 1e-4f;
    inline constexpr f32 VERY_SMALL_NUMBER = 1e-6f;
    inline constexpr f32 KINDA_LIKE_ONE = 0.99f;

} // namespace Medium::Math

namespace Medium::Math {

    // 判定浮点数是否接近零
    inline bool isNearlyZero(f32 value, f32 tolerance = KINDA_SMALL_NUMBER) noexcept {
        return std::abs(value) <= tolerance;
    }

    // 度数转弧度
    constexpr f32 deg_to_rad(f32 deg) {
        return MEDIUM_PI * deg / 180.0f;
    }

    // 弧度转度数
    constexpr f32 rad_to_deg(f32 rad) {
        return rad * 180.0f / MEDIUM_PI;
    }

    /// @brief 欧拉角转四元数
    /// @param    euler_deg 欧拉角（度数）Pitch(X), Heading(Y), Bank(Z)
    /* 1. AngleAxis 是旋转的一种「中间表达式」，其对乘号做了重载，相乘隐式得到一个 Quaternion.
     * 2. 绕世界固定轴 Z-X-Y 旋转等价于绕对象轴 Y-X-Z 旋转.
     * 3. 复合旋转为右结合：rot * v = Heading * (Pitch * (Bank * v))
     *    固定轴视角为 Bank(Z) -> Pitch(X) -> Heading(Y)，局部轴视角为 Yaw(Y) -> Pitch(X) -> Roll(Z)
     * 4. 该项目采用 FPS 控制，固采用 YXZ 的顺序旋转。                                             */
    inline Quatf eulerToQuatf(const Vec3f& euler_deg) {
        f32 x_rad = deg_to_rad(euler_deg.x());
        f32 y_rad = deg_to_rad(euler_deg.y());
        f32 z_rad = deg_to_rad(euler_deg.z());

        using namespace Eigen;
        Quatf rot = AngleAxisf(y_rad, Vec3f::UnitY())
                  * AngleAxisf(x_rad, Vec3f::UnitX())
                  * AngleAxisf(z_rad, Vec3f::UnitZ());

        return rot;
    }

    /// @brief 普通线性插值；
    /// @param attrib0, attrib1, attrib2 三个待混合顶点属性；
    /// @param weights 重心坐标 (alpha, beta, gamma)；
    /// @return 混合结果。
    template <typename T>
    T interpolate(const T& attrib0, const T& attrib1, const T& attrib2, const Vec3f& weights) noexcept {
        return attrib0 * weights.x() + attrib1 * weights.y() + attrib2 * weights.z();
    }

    /// @brief 透视矫正插值；
    /// @param attrib0, attrib1, attrib2 三个待混合顶点属性；
    /// @param weights 重心坐标 (alpha, beta, gamma)；
    /// @param w_invs 三个顶点的 1/w 值，已存入顶点着色器里；
    /// @note 要求类型 T 必须支持 * 与 + 运算符重载
    /// @details 1/w 与 attrib/w 在屏幕空间均为线性的。要得到当前像素的 attrib_interp，只需对三个顶点的 attrib/w 进行
    /// 插值，再除以当前像素的 1/w（也为插值得到）即得 attrib_interp;
    /// @return 混合结果。
    template <typename T>
    T interpolate(const T& attrib0, const T& attrib1, const T& attrib2, const Vec3f& weights, const Vec3f& w_invs) {
        // 中间值，用于避免冗余乘法
        f32 tmp0 = w_invs[0] * weights[0];
        f32 tmp1 = w_invs[1] * weights[1];
        f32 tmp2 = w_invs[2] * weights[2];
        // 插值当前像素的 1/w，即透视矫正公式的分母
        f32 w_inv_interp = tmp0 + tmp1 + tmp2;
        // 避免除零，w_inv_interp 极小说明像素位于"无穷远"
        if(std::abs(w_inv_interp) < 1e-5f) return attrib0;
        // 透视矫正公式的分子，对三个顶点的 attrib/w 进行插值
        T val = (attrib0 * tmp0) + (attrib1 * tmp1) + (attrib2 * tmp2);
        // 当前像素的 w，透视矫正公式分母的倒数
        f32 w_interp = 1.0f / w_inv_interp;
        // 除以 1/w 等价于乘以 w
        return val * w_interp;
    }

} // namespace Medium::Math




