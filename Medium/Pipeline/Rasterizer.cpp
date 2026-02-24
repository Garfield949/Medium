///
/// @file Rasterizer.cpp
///
#include "Rasterizer.h"

#include "Pipeline/RenderTarget.h"
#include "Shader/Shaders.h"
#include "Resources/Mesh.h"

#include "Core/Types.h"
#include "Core/Math/Math.h"
#include "Core/Math/Geometry.h"

namespace Medium {

    // 实现 Rasterizer 成员函数 draw
    void Rasterizer::draw(const Mesh &mesh, const SubMesh &sub_mesh,
                          IShader &shader,  RenderTarget &render_target) {

        // 三角面片的三个顶点
        std::array<Vec4f, 3> tri;

        // 每次取 3 个顶点作为一个面片
        for(auto i = 0; i < sub_mesh.triangleCount(); i ++) {
            auto tri_idx = sub_mesh.index_offset + (i * 3);

            // 运行顶点着色器 World Space -> Clip Space
            for(int j = 0; j < 3; j++)
                tri[j] = shader.vertex(j, mesh.vertices[mesh.indices[tri_idx + j]]);

            // 简单的全三角形剔除：只要有一个点在近平面后面或太近，就丢弃。
            // 这里取 0.1f 作为近平面阈值
            if(tri[0].w() < 0.1f || tri[1].w() < 0.1f || tri[2].w() < 0.1f) {
                continue;
            }

            // TODO: 裁剪，后续再实现。

            // 透视除法 Clip Space -> NDC. 注意 w 分量赋值为 1/w(1/z)
            // x,y -> [-1,1], z -> [0,1]
            for(auto & coord : tri) {
                f32 w_inv = 1.0f / coord.w();
                coord *= w_inv;
                coord.w() = w_inv;
            }

            // 视口变换 NDC -> Screen Space
            // [-1,1] 映射到 [0, width], [0, height]
            for(auto & coord : tri) 
                vpTransform(coord, render_target.width(), render_target.height());

            // 屏幕空间背面剔除 Back-face Culling
            // 有向面积为正说明三角形是逆时针的，这在左手系中是背对相机的
            if (orient2D(tri[0], tri[1], tri[2]) > 0) continue;

            // 屏幕坐标光栅化
            rasterizeTri(tri, shader, render_target);
        }
    }

    // 实现 Rasterizer 成员函数 rasterizeTri
    void Rasterizer::rasterizeTri(const std::array<Vec4f, 3>& tri, IShader& shader, RenderTarget& render_target) {
        // 计算包围盒
        BBox2D bbox = BBox2D::fromTriangle(tri, render_target.width(), render_target.height());
        // 若三角形完全在屏幕外，直接丢弃
        if(!bbox.valid()) return;

        // 遍历面片的包围盒
        for(u32 x = bbox.min.x(); x <= bbox.max.x(); x++) {
            for(u32 y = bbox.min.y(); y <= bbox.max.y(); y++) {
                // 计算重心坐标，测试物理像素中心 (x+0.5, y+0.5) 是否在三角形内
                // TODO: 搞清楚物理中心的意义
                auto weights = barycentric(tri[0], tri[1], tri[2], (f32)x+0.5f, (f32)y+0.5f);
                if(weights.x() < 0 || weights.y() < 0 || weights.z() < 0)
                    continue;

                // 注：Zndc 已经是 A+B/z 的形式，在屏幕空间是线性的，无需透视矫正插值。
                f32 z_interpolated = Math::interpolate(tri[0].z(), tri[1].z(), tri[2].z(), weights);

                // Early-Z
                if(render_target.updateDepth(x, y, z_interpolated)) {
                    Color pixel_color;
                    bool discard = shader.fragment(weights, pixel_color);
                    if(!discard)
                        render_target.setColor(x, y, pixel_color);
                }
            }
        }

    }

} // namespace Medium