///
/// @file   Pipeline.cpp
/// @author Garfield949
/// @date   2026/3/1 20:57
/// @brief
///
#include "Pipeline.h"

#include <fmt/base.h>

#include "Base/Types.h"

#include "Resources/Mesh.h"

#include "Geometry/AABB.h"
#include "Geometry/Geometry.h"
#include "Geometry/Subpixel.h"

#include "Pipeline/Clip.h"
#include "Pipeline/RenderTarget.h"
#include "Pipeline/Shader/Shader.h"
#include "Pipeline/RenderContext.h"

namespace {
  using namespace Medium;
  /// @brief   视口变换
  /// @details 接收一个 NDC 坐标，将其 x,y 分量映射到 [0,width] [0,height]
  void NDCToScreen(Vec4f& coord, u32 sc_width, u32 sc_height) {
    coord.x() = 0.5f * static_cast<f32>(sc_width)  * (coord.x() + 1.0f);
    coord.y() = 0.5f * static_cast<f32>(sc_height) * (coord.y() + 1.0f);
  }
}

namespace Medium::Pipeline {

  void executeQueue(const PassCtx& ctx, const std::vector<RenderCommand>& cmds) {
    std::visit([&](auto& active_shader) {
      for(const auto & cmd : cmds) {
        active_shader.bind(ctx, *cmd.matrix_world, cmd.material);
        draw(*cmd.mesh, *cmd.submesh, ctx.shader, ctx.buffer);
      }
    }, *ctx.shader);
  }

  void draw(const Mesh &mesh, const SubMesh &sub_mesh, VariantShader* shader, RenderTarget* buffer) {
    std::visit([&](auto& active_shader) { // SubMesh 层级做 dispatch

      for(size_t i = 0; i < sub_mesh.triangleCnt(); ++i) {
        // Mesh 每 3 个紧邻的顶点索引为一个三角形
        size_t tri_idx = sub_mesh.index_offset + (i * 3);

        // 将当前三角形作为裁剪输入，运行顶点着色器 World Space -> Clip Space，并获取待插值的 varying
        ClipPolygon clip_poly;
        for(size_t j = 0; j < 3; ++j, ++clip_poly.num_verts) {
          size_t vert_idx = mesh.indices[tri_idx + j];
          
          VSOut vs_out = active_shader.vertex(mesh.vertices[vert_idx]);
          
          clip_poly.verts[j].pos     = vs_out.pos_clip;
          clip_poly.verts[j].varying = vs_out.varying;
        }

        if(!clip(clip_poly)) continue;

        // 透视除法 Clip Space -> NDC. x,y -> [-1,1], reversed-z -> [0,1]
        for(size_t k = 0; k < clip_poly.num_verts; ++k) {
          Vec4f& pos = clip_poly.verts[k].pos;
          f32 w_inv = 1.0f / pos.w();
          pos.x() *= w_inv;
          pos.y() *= w_inv;
          pos.z() *= w_inv;
        }

        // 扇形划分：n 个顶点的凸多边形可分出 n - 2 个三角形
        for(size_t l = 1; l + 1 < clip_poly.num_verts; ++l) {
          std::array<Vec4f, 3> triangle = {
            clip_poly.verts[0].pos, clip_poly.verts[l].pos, clip_poly.verts[l + 1].pos
          };
          // 背面剔除 Back-face Culling. NDC 空间和屏幕空间的剔除效果一致，所以在视口变换前先做剔除，免得白做一次视口变换；
          // 有向面积为正说明三角形是逆时针的，这在左手系中是背对相机的；
          // 由于这里已经剔除掉了所有逆时针三角形，所以后续计算 Edge Function 时只用处理顺时针三角形。
          if (Geometry::orient2D(triangle[0], triangle[1], triangle[2]) > 0) continue;
          // 视口变换 NDC -> Screen Space; 顶点的 x,y 由 [-1.0,1.0] 映射到 [0.0, width], [0.0, height].
          for(auto & coord : triangle)
            NDCToScreen(coord, buffer->width(), buffer->height());

          std::array<Varying, 3> varyings {
            clip_poly.verts[0].varying, clip_poly.verts[l].varying, clip_poly.verts[l + 1].varying
          };

          // 将屏幕坐标送进光栅器。
          rasterize(triangle, varyings, shader, buffer);
        }
      }
      
    }, *shader); // std::visit
  } // draw

  // 边函数法，Edge Equation.
  // E(x,y) = (x-x0)(y1-y0) - (y-y0)(x1-x0)
  //        = (y1-y0)x - (x1-x0)y + y0(x1-x0) - x0(y1-y0)
  //        = Ax + By + C.
  // 故当 E(x+1,y) 有 Delta E = A = y1-y0；当 E(x,y+1) 有 Delta E = B = -(x1-x0) = x0-x1.
  // 注：当应用了次像素，边函数就变为了 E(x*scale, y*scale) = (A*scale)x + (B*scale)y + C.
  void rasterize(const std::array<Vec4f, 3>&   tri_screen,
                 const std::array<Varying, 3>& varyings,
                 VariantShader* shader,
                 RenderTarget*  buffer)
  {
    using namespace Geometry;

    std::array<Vec2i, 3> tri_fixed; // 将传入三角形的浮点屏幕坐标提升为次像素定点数；
    std::array<f32  , 3> v_depth{}; // 保存三角形顶尖的深度值 Zndc.

    for(i32 i = 0; i < 3; ++i) {
      tri_fixed[i].x() = Subpixel::fromFloat(tri_screen[i].x());
      tri_fixed[i].y() = Subpixel::fromFloat(tri_screen[i].y());
      v_depth[i]       = tri_screen[i].z();
    }

    ScreenAABB bbox = ScreenAABB::fromSubpixelTri(tri_fixed, buffer->width(), buffer->height());
    if(!bbox.valid()) return; // 若三角形完全在屏幕外，直接丢弃

    // 中间量语义化
    Vec2i delta01{tri_fixed[1].x() - tri_fixed[0].x(),  tri_fixed[1].y() - tri_fixed[0].y()};
    Vec2i delta12{tri_fixed[2].x() - tri_fixed[1].x(),  tri_fixed[2].y() - tri_fixed[1].y()};
    Vec2i delta20{tri_fixed[0].x() - tri_fixed[2].x(),  tri_fixed[0].y() - tri_fixed[2].y()};

    // A 对应边函数 E(X+1,Y) 时的增量；同理，B 对应 E(X,Y+1) 时的增量
    i32 A01 = Subpixel::Scale * delta01.y(),  B01 = -Subpixel::Scale * delta01.x();
    i32 A12 = Subpixel::Scale * delta12.y(),  B12 = -Subpixel::Scale * delta12.x();
    i32 A20 = Subpixel::Scale * delta20.y(),  B20 = -Subpixel::Scale * delta20.x();

    // 从包围盒左下角的坐标开始测试、着色，一行一行往上
    // 加一个偏置将采样点移到像素中心，这里 array 方法将向量转为数组以进行逐分量加法
    Vec2i p_start = (bbox.min * Subpixel::Scale).array() + Subpixel::Half;

    // 开头获取首坐标对应三条边的 Equation，后续的坐标只需在此基础上加上推导好的增量即可。
    i64 E01_row = edgeEquation(tri_fixed[0],  tri_fixed[1], p_start);
    i64 E12_row = edgeEquation(tri_fixed[1],  tri_fixed[2], p_start);
    i64 E20_row = edgeEquation(tri_fixed[2],  tri_fixed[0], p_start);

    i64 area_total = E01_row + E12_row + E20_row;
    if(area_total == 0) return; // 丢弃退化三角形

    f32 area_total_inv = 1.0f / static_cast<f32>(area_total);

    // 缓存 Top-Left 的偏置值
    i32 bias01 = topLeftBias(delta01.x(),  delta01.y());
    i32 bias12 = topLeftBias(delta12.x(),  delta12.y());
    i32 bias20 = topLeftBias(delta20.x(),  delta20.y());

    // Top-Left Rule 的偏置在计算完面积后再加，否则面积计算会受影响
    E01_row += bias01;
    E12_row += bias12;
    E20_row += bias20;

    // 三角形层面做 dispatch
    std::visit([&](auto& active_shader) {
      // 边函数增量更新
      for(i32 y = bbox.min.y(); y <= bbox.max.y(); y++) {
        // 当前像素的 Equation 值，注意同样要采用 i64.
        i64 E01 = E01_row; // 三角形 V0V1P 面积的二倍
        i64 E12 = E12_row; // 三角形 V1V2P 面积的二倍
        i64 E20 = E20_row; // 三角形 V2V0P 面积的二倍
        for(i32 x = bbox.min.x(); x <= bbox.max.x(); x++) {
          // 任意一个为负，其符号位为 1，OR 后整体为负。
          if((E01 | E12 | E20) >= 0) {
            // Equation 值需临时减去偏置，重心坐标需要精确的值
            // V0 对应对边 E12，其它同理
            Vec3f weights{
              static_cast<f32>(E12-bias12) * area_total_inv,
              static_cast<f32>(E20-bias20) * area_total_inv,
              static_cast<f32>(E01-bias01) * area_total_inv
            };

            // Zndc 已是 A+B/z 的形式，在屏幕空间是线性的，无需透视矫正插值。
            f32 z_interp = Math::interpolate(v_depth[0], v_depth[1], v_depth[2], weights);

            // Early-Z
            u32 index = buffer->getBufIdx(x, y);
            if(buffer->depthTest(index, z_interp)) {
              LinearColor frag_color;
              // Alpha test
              bool discard = active_shader.fragment(varyings, weights, frag_color);
              if(!discard) {
                buffer->writeDepth(index, z_interp);
                buffer->setColor(index, frag_color);
              }
            }
          }
          //  E(X+1,Y) 方向推进
          E01 += A01;
          E12 += A12;
          E20 += A20;
        }
        // 换行，E(X,Y+1) 方向推进
        E01_row += B01;
        E12_row += B12;
        E20_row += B20;
      }
    }, *shader);

  } // rasterize

} // namespace Medium::Pipeline
