///
/// @file Rasterizer.h
/// @brief 光栅化器
/// @details
///
#pragma once

#include "Core/Types.h"

namespace Medium {
    // 前向声明
    class  RenderTarget;
    class  Mesh;
    struct SubMesh;
    struct IShader;
} // namespace Medium

namespace Medium {

    /// @class Rasterizer
    /// @brief 管理渲染管线核心部分
    /// @details 管理 draw call 流程的逻辑，具体纯几何运算交给 Geometry.h
    class Rasterizer {
    public:
        Rasterizer() = default;

        // 绘制入口，传入顶点汤与着色器
        void draw(const Mesh &mesh, const SubMesh &sub_mesh,
                  IShader &shader,  RenderTarget &render_target);

    private:
        // 光栅化单个三角面片
        void rasterizeTri(const std::array<Vec4f, 3>& tri, IShader& shader, RenderTarget& render_target);

        /// @brief 视口变换函数 Viewport Transform
        /// @details 接收一个 NDC 坐标，将其 x,y 分量映射到 [0,width] [0,height]
        void vpTransform(Vec4f& coord, u32 vp_width, u32 vp_height) {
            coord.x() = 0.5f * static_cast<float>(vp_width)  * (coord.x() + 1.0f);
            coord.y() = 0.5f * static_cast<float>(vp_height) * (coord.y() + 1.0f);
        }
    };

} // Medium