///
/// @file   IShader.h
/// @author Garfield949
/// @date   2026/2/13 15:07
/// @brief  
///
#pragma once

#include "Base/Types.h"
#include "Base/Primitives/Vertex.h"
#include "Resources/Material.h"

namespace Medium {

    enum class ColorSource {
        Checkerboard, Textured, SolidColor
    };

    // Varying 是单个 draw call 里不断被插值变化的量，描述了属性如何从顶点的“点数据”
    // 演变成覆盖整个三角形面的“面数据”。
    struct Varying {
        Vec2f uv;        // 纹理坐标
        Vec3f normal;    // 顶点变到世界空间里的法向
        Vec3f pos_world; // 某些计算（比如光照）依赖顶点世界坐标
        f32   w_inv;     // 保存顶点的 1/w
    };

    // 单个 draw call 中不变的量
    struct Uniform {
        Mat4f world;
        Mat4f wvp;
        Vec3f camera_pos; // 用于高光着色
        Vec3f light_dir;

        MaterialParams mat_params;

        const Texture* map_Kd = nullptr;
        const Texture* map_Ks = nullptr;

        ColorSource color_source_diffuse  = ColorSource::Checkerboard;
        ColorSource color_source_specular = ColorSource::Checkerboard;
    };

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
        virtual bool fragment(const Vec3f& weights, Color& color_final) = 0;

        std::array<Varying, 3> varyings;
        Uniform uniforms;
    };

} // namespace Medium
