///
/// @file Geometry.h
/// @brief 放置通用的几何、计算相关的类型与函数
///
#pragma once

#include "Math.h"
#include "Core/Types.h"

namespace Medium {

    /// @struct BBox2D
    /// @brief 屏幕空间包围盒类型
    /// @details A, B, C 为三角形屏幕坐标，P 为当前要判断的像素点
    struct BBox2D {
        BBox2D() = default;

        /// @brief BBox2D 成员函数，判断包围盒是否有效
        [[nodiscard]] constexpr bool valid() const noexcept {
            return min.x() <= max.x() && min.y() <= max.y();
        }

        /// @brief BBox2D 成员函数，接收三角面片顶点、屏幕宽高
        static BBox2D fromTriangle(const std::array<Vec4f, 3>& tri, u32 width, u32 height) {
            BBox2D bbox;

            // 先找出面片的边界；
            // std::minmax 返回一个 std::pair，第一个为最小值，第二个为最大值；
            // C++11：传的参数为一个初始化列表，编译器在背后隐式地在栈上创建一个数组；C++17：结构化绑定语法，允许将某些类型的成员解包到独立的变量中；
            // 注意：minmax 的参数为初始化列表时，返回值为值拷贝；若使用双参数版本 std::minmax(a,b)，auto [res1, res2] 接收的将是临时变量 a
            // 和 b 的常量引用，会造成引用悬垂错误。
            auto [min_x, max_x] = std::minmax({tri[0].x(), tri[1].x(), tri[2].x()});
            auto [min_y, max_y] = std::minmax({tri[0].y(), tri[1].y(), tri[2].y()});

            // 转换为整数，并进行安全的取整策略：宁可多框一些像素，多余的用重心坐标测试裁掉，避免漏掉边缘像素
            // floor: 保证 min 有 10.9 -> 10
            // ceil:  保证 max 有 10.1 -> 11
            int i_min_x = static_cast<int>(std::floor(min_x));
            int i_max_x = static_cast<int>(std::ceil(max_x));
            int i_min_y = static_cast<int>(std::floor(min_y));
            int i_max_y = static_cast<int>(std::ceil(max_y));

            // 计算与屏幕矩形的交集
            bbox.min.x() = std::max(0, i_min_x);
            bbox.min.y() = std::max(0, i_min_y);
            bbox.max.x() = std::min(static_cast<int>(width - 1),  i_max_x);
            bbox.max.y() = std::min(static_cast<int>(height - 1), i_max_y);

            return bbox;
        }

        // 使用 int 是因为非法 BBOX 可能会有负值
        Vec2i min; // 左下角
        Vec2i max; // 右上角
    }; // struct BBox2D

} // namespace Medium

namespace Medium {

    /// @brief 屏幕空间重心坐标函数
    /// @details A, B, C 为三角形屏幕坐标，P 为当前要判断的像素点
    /// @return 返回 A, B, C 对应重心坐标
    inline Vec3f barycentric(const Vec4f &A, const Vec4f &B, const Vec4f &C, f32 px, f32 py) {
        Vec3f v_x(C.x() - A.x(), B.x() - A.x(), A.x() - px);
        Vec3f v_y(C.y() - A.y(), B.y() - A.y(), A.y() - py);

        // 注意：上方构造 v_x 和 v_y 的顺序决定了 beta 在 u.y，gamma 在 u.x
        Vec3f u = v_x.cross(v_y);

        // 若 u.z 绝对值太小，说明三角形退化，面积为 0
        // u.z 即三角面片有符号面积的二倍，想更激进剔除不可见的微小三角形，这个浮点数判零的阈值还可调得更大（太大会导致丢失很多细微三角形）
        // 返回值带负分量，让后续逻辑剔除
        if (std::abs(u.z()) < Math::KINDA_SMALL_NUMBER) return {-1, 1, 1};

        // 若 P 在三角形外面，则返回值至少有一个分量是负数
        // 返回 alpha, beta, gamma
        return {1.f - (u.x() + u.y()) / u.z(), u.y() / u.z(), u.x() / u.z()};
    }

    /// @brief 计算 2D 向量叉积，orient2D 的辅助函数
    /// @details 叉积结果为向量所围平行四边形的有向面积
    inline f32 cross2D(const Vec2f& v0, const Vec2f& v1) {
        return v0.x() * v1.y() - v1.x() * v0.y();
    }

    /// @brief 接收三角面片的三个顶点 a, b, c，计算其在 2D 空间内的朝向
    /// @details 计算向量 (b-a) 与 (c-a) 的叉积
    /// @return > 0 为 CCW，< 0 为 CW，=0 为共线面积为 0
    inline f32 orient2D(const Vec4f& a, const Vec4f& b, const Vec4f& c) {
        Vec2f v0 = {b.x() - a.x(), b.y() - a.y()};
        Vec2f v1 = {c.x() - a.x(), c.y() - a.y()};
        return cross2D(v0, v1);
    }

} // namespace Medium
