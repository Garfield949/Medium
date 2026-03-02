///
/// @file   IShader.h
/// @author Garfield949
/// @date   2026/2/13 15:07
/// @brief
///
#pragma once

#include "Base/Types.h"
#include "Base/Primitives/Vertex.h"

namespace Medium {

    /// @class IShader
    /// @brief 着色器接口类型
    /// @details 定义顶点、片元着色器的虚拟接口
    struct IShader {
        IShader() = default;
        // 析构函数声明为 virtual，当通过 IShder* 管理某派生类型的实例
        // 时，delete 会调用派生类型的析构函数。
        virtual ~IShader() = default;

        /// @brief 顶点着色器
        /// @param nth_vert 当前处理的是三角面片第几个顶点
        /// @param vert 顶点数据
        /// @details 写入 varyings
        /// @return 返回当前顶点的裁剪空间形式
        virtual Vec4f vertex(u32 nth_vert, const Vertex& vert) = 0;

        /// @brief 片元着色器
        /// @param weights 重心坐标 (alpha, beta, gamma)
        /// @param color_final 输出颜色
        /// @return true 若需丢弃该像素，false 保留
        /// @details 利用 weights 对 varying 进行插值
        virtual bool fragment(const Vec3f& weights, LinearColor& color_final) = 0;
    };

} // namespace Medium