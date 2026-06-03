///
/// @file   Pipeline.cpp
/// @author Garfield949
/// @date   2026/3/1 20:57
/// @brief
///
#include "Pipeline.h"

#include "Base/Geometry/Geometry.h"
#include "Base/Geometry/Rect.h"
#include "Resources/Mesh.h"
#include "Pipeline/Subpixel.h"
#include "Pipeline/Clip.h"
#include "Pipeline/RenderTarget.h"
#include "Pipeline/RenderContext.h"

namespace {
  
using namespace Medium;

/// @brief   视口变换。
/// @details 接收一个 NDC 坐标，将其 x,y 分量映射到以给定视口。
void NDCToViewport(Vec4f& vert, const RectF& viewport) {
  vert.x() = viewport.x + 0.5f * viewport.w * (vert.x() + 1.0f);
  vert.y() = viewport.y + 0.5f * viewport.h * (vert.y() + 1.0f);
}

/// @brief 计算三角形的屏幕包围盒。
/// @details Viewport 的作用是进行边界截断，会从 rasterizer 传入一个 RectF 自动隐式转型。
Rect calcTriBBox(const std::array<Vec2i, 3>& tri_subpixel, const Rect& viewport)
{
  // 获取次像素空间的包围盒边界。
  i32 x_min_sub = std::min({tri_subpixel[0].x(), tri_subpixel[1].x(), tri_subpixel[2].x()});
  i32 x_max_sub = std::max({tri_subpixel[0].x(), tri_subpixel[1].x(), tri_subpixel[2].x()});
  i32 y_min_sub = std::min({tri_subpixel[0].y(), tri_subpixel[1].y(), tri_subpixel[2].y()});
  i32 y_max_sub = std::max({tri_subpixel[0].y(), tri_subpixel[1].y(), tri_subpixel[2].y()});

  // 将边界从次像素空间转回像素空间。进行安全的取整策略：宁可多框一些像素，避免漏掉边缘像素。
  // - Floor   行为：单纯右移，等价于除以 16. 使得在 [160, 176) 内的值取整结果为 10;
  // - Ceiling 行为：加一个 bias 再右移。这里加 15，使得在 (160, 176] 内的值取整结果为 11;
  // - 再针对 viewport 的边界值截断一下。
  i32 bias  = Subpixel::kScale - 1;
  i32 x_min = std::max(viewport.x, x_min_sub >> Subpixel::kBits);
  i32 y_min = std::max(viewport.y, y_min_sub >> Subpixel::kBits);
  i32 x_max = std::min((x_max_sub + bias) >> Subpixel::kBits, viewport.x + viewport.w - 1);
  i32 y_max = std::min((y_max_sub + bias) >> Subpixel::kBits, viewport.y + viewport.h - 1);

  // 在视区外部的三角形，其边界经过截断后，算出的包围盒宽度和高度一定 <= 0，即判定非法。
  // 举例：假设视区原点 (0,0)，宽度 = 100，x 边界为 100 - 1 = 99.
  // 1) x_min = -10  钳制为 0;   x_max = -2  保持为 -2.  则 rect.w = -2  - 0   + 1 = -1 <  0 非法；
  // 2) x_min = -2   钳制为 0;   x_max = -1  保持为 -1.  则 rect.w = -1  - 0   + 1 = 0  <= 0 非法；
  // 3) x_min =  2   保持为 2;   x_max =  2  保持为  2.  则 rect.w =  2  - 2   + 1 = 1  >  0 合法；
  // 4) x_min =  100 保持为 100; x_max = 100 钳制为  99. 则 rect.w =  99 - 100 + 1 = 0  <= 0 非法；
  Rect rect;
  rect.x = x_min;
  rect.y = y_min;
  rect.w = x_max - x_min + 1;
  rect.h = y_max - y_min + 1;

  return rect;
}

}

namespace Medium::Pipeline {

void executeCommands(const std::vector<RenderCommand>& cmds,
                     const ShaderBindCtx& shader_ctx,
                     const RenderPassCtx& pass_ctx,
                     VariantShader&       shader)
{
  std::visit([&](auto& active_shader) {
    for(const auto& cmd: cmds) {
      active_shader.bind(shader_ctx, cmd.mat_world, cmd.material);
      draw(*cmd.mesh, *cmd.submesh, pass_ctx.viewport, pass_ctx.buffer, shader);
    }
  }, shader);
}

void draw(const Mesh&    mesh,
          const SubMesh& sub_mesh,
          const RectF&   viewport,
          RenderTarget&  buffer,
          VariantShader& shader)
{
  std::visit([&](auto& active_shader) { // SubMesh 层级做 dispatch

    for(size_t i = 0; i < sub_mesh.triangleCount(); ++i) {
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
        // TODO: 此处的 culling 或许能变成可配置的 -- 背面、正面、不剔除；
        // 背面剔除 Back-face Culling. NDC 空间和屏幕空间的剔除效果一致，所以在视口变换前先做剔除，免得白做一次视口变换；
        // 有向面积为正说明三角形是逆时针的，这在左手系中是背对相机的；
        // 由于这里已经剔除掉了所有逆时针三角形，所以后续计算 Edge Function 时只用处理顺时针三角形。
        if (Geometry::orient2D(triangle[0], triangle[1], triangle[2]) > 0) continue;
        // 视口变换 NDC -> Screen Space; 顶点的 x,y 由 [-1.0,1.0] 映射到 [0.0, width], [0.0, height].
        for(auto& vert: triangle)
          NDCToViewport(vert, viewport);

        std::array<Varying, 3> varyings {
          clip_poly.verts[0].varying, clip_poly.verts[l].varying, clip_poly.verts[l + 1].varying
        };
        // 将屏幕坐标送进光栅器。
        rasterize(triangle, varyings, viewport, buffer, shader);
      }
    }

  }, shader); // std::visit
} // draw

// 边函数法，Edge Equation.
// E(x,y) = (x-x0)(y1-y0) - (y-y0)(x1-x0)
//        = (y1-y0)x - (x1-x0)y + y0(x1-x0) - x0(y1-y0)
//        = Ax + By + C.
// 故当 E(x+1,y) 有 Delta E = A = y1-y0；当 E(x,y+1) 有 Delta E = B = -(x1-x0) = x0-x1.
// 注：当应用了次像素，边函数就变为了 E(x*scale, y*scale) = (A*scale)x + (B*scale)y + C.
void rasterize(const std::array<Vec4f, 3>&   tri_screen,
               const std::array<Varying, 3>& varyings,
               const RectF&   viewport,
               RenderTarget&  buffer,
               VariantShader& shader)
{
  using namespace Geometry;

  std::array<Vec2i, 3> tri_subpixel{}; // 将传入三角形的浮点屏幕坐标提升为次像素定点数；
  std::array<f32  , 3> vert_depth{};   // 保存三角形顶点的深度值 Zndc.

  // 像素空间 -> 次像素空间
  for(i32 i = 0; i < 3; ++i) {
    tri_subpixel[i].x() = Subpixel::fromFloat(tri_screen[i].x());
    tri_subpixel[i].y() = Subpixel::fromFloat(tri_screen[i].y());
    vert_depth[i]       = tri_screen[i].z();
  }

  Rect bbox_tri = calcTriBBox(tri_subpixel, viewport);
  if(!bbox_tri.valid()) return; // 若三角形完全在视区外，直接丢弃

  // 中间量语义化
  Vec2i delta01{tri_subpixel[1].x() - tri_subpixel[0].x(),  tri_subpixel[1].y() - tri_subpixel[0].y()};
  Vec2i delta12{tri_subpixel[2].x() - tri_subpixel[1].x(),  tri_subpixel[2].y() - tri_subpixel[1].y()};
  Vec2i delta20{tri_subpixel[0].x() - tri_subpixel[2].x(),  tri_subpixel[0].y() - tri_subpixel[2].y()};

  // A 对应边函数 E(X+1,Y) 时的增量；同理，B 对应 E(X,Y+1) 时的增量
  i32 A01 = Subpixel::kScale * delta01.y(),  B01 = -Subpixel::kScale * delta01.x();
  i32 A12 = Subpixel::kScale * delta12.y(),  B12 = -Subpixel::kScale * delta12.x();
  i32 A20 = Subpixel::kScale * delta20.y(),  B20 = -Subpixel::kScale * delta20.x();

  // 从包围盒左下角的坐标开始测试、着色，一行一行往上
  // 加一个偏置将采样点移到像素中心，这里 array 方法将向量转为数组以进行逐分量加法
  Vec2i p_start = (bbox_tri.min() * Subpixel::kScale).array() + Subpixel::kHalf;

  // 开头获取首坐标对应三条边的 Equation，后续的坐标只需在此基础上加上推导好的增量即可。
  i64 E01_row = edgeEquation(tri_subpixel[0],  tri_subpixel[1], p_start);
  i64 E12_row = edgeEquation(tri_subpixel[1],  tri_subpixel[2], p_start);
  i64 E20_row = edgeEquation(tri_subpixel[2],  tri_subpixel[0], p_start);

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
    for(i32 y = bbox_tri.min().y(); y <= bbox_tri.max().y(); y++) {
      // 当前像素的 Equation 值，注意同样要采用 i64.
      i64 E01 = E01_row; // 三角形 V0V1P 面积的二倍
      i64 E12 = E12_row; // 三角形 V1V2P 面积的二倍
      i64 E20 = E20_row; // 三角形 V2V0P 面积的二倍
      for(i32 x = bbox_tri.min().x(); x <= bbox_tri.max().x(); x++) {
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
          f32 z_interp = Math::interpolate(vert_depth[0], vert_depth[1], vert_depth[2], weights);

          // Early-Z
          u32 index = buffer.getBufIdx(x, y);
          if(buffer.depthTest(index, z_interp)) {
            LinearColor frag_color;
            // Alpha test
            bool discard = active_shader.fragment(varyings, weights, frag_color);
            if(!discard) {
              buffer.writeDepth(index, z_interp);
              buffer.setColor(index, frag_color);
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
  }, shader);

} // rasterize

} // namespace Medium::Pipeline
