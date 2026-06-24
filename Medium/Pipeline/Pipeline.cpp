///
/// @file   Pipeline.cpp
/// @author Garfield949
/// @date   2026/3/1 20:57
/// @brief
///
#include "Pipeline.h"

#include "Base/Math/Math.h"
#include "Base/Geometry/Rect.h"
#include "Base/Math/Interpolation.h"
#include "Pipeline/RenderCommand.h"
#include "Pipeline/Subpixel.h"
#include "Pipeline/Clip.h"
#include "Pipeline/FrameBuffer.h"
#include "Pipeline/Shader/ShaderTypes.h"
#include "Pipeline/RenderContext.h"
#include "Resources/Mesh.h"

namespace Medium {
namespace {
// 透视除法 x,y -> [-1,1], reversed-z -> [0,1].
// 正交投影 w 恒为 1，可以不走这个循环，但非热点不用管。
void clipToNDC(ClipPolygon& clip_poly) {
  for(u32 k = 0; k < clip_poly.num_verts; ++k) {
    Vec4f& pos = clip_poly.verts[k].pos;
    f32 w_inv = 1.0f / pos.w();
    pos.x() *= w_inv;
    pos.y() *= w_inv;
    pos.z() *= w_inv;
  }
}

/// @brief 接收三角面片的三个顶点，计算其在 2D 空间内的朝向；
/// @details 计算向量 (b-a) 与 (c-a) 的叉积；
///          1. 叉积代数定义：(x1,y1,z1) x (x2,y2,z2) = (y1z2-z1y2, x2z1-x1z2, x1y2-x2y1). 所以 2D 叉积就是 3D 
///             向量 (x1,y1,0) 与 (x2,y2,0) 叉积所得向量 (0,0,x1y2-x2y1) 的 z 值。
///          2. 对于处于 NDC / 屏幕空间（该项目管线的屏幕坐标原点同样为左下角）的三角形，从 2D (xy轴) 视角看会有两种绕序：
///             1) 顺时针 CW: 若边向量 v2 在 v1 的「右边」，则叉积结果一定 <= 0. 换句话说，叉积所得向量的 z 值为非正数。
///                而在左手系下约定 +z 轴指向屏幕内，所以绕序为 CW 的三角形的正面是朝向屏幕外的。
///             2) 逆时针 CCW: 若边向量 v2 在 v1 的「左边」，则叉积结果一定 >= 0.对于左手系来说这样的三角形正面是指向屏
///                幕内的；而右手系规定 -z 指向屏幕内，故绕序为 CCW 的三角形反而在这种约定下正面朝向屏幕外了。
/// @return CW < 0; CCW > 0; = 0 代表两边共线，三角形退化面积为 0.
f32 orient2D(const Vec4f& a, const Vec4f& b, const Vec4f& c) noexcept {
  Vec2f v1 = b.head<2>() - a.head<2>();
  Vec2f v2 = c.head<2>() - a.head<2>();
  return Math::cross2D(v1, v2);
}

/// @brief   视口变换。
/// @details 接收一个 NDC 坐标，将其 x,y 分量映射到以给定视口。
void NDCToViewport(Vec4f& vert, const RectF& viewport) {
  vert.x() = viewport.x + 0.5f * viewport.w * (vert.x() + 1.0f);
  vert.y() = viewport.y + 0.5f * viewport.h * (vert.y() + 1.0f);
}

/// @brief 计算三角形的屏幕包围盒。
/// @details Viewport 的作用是进行边界截断，会从 rasterizer 传入一个 RectF 自动隐式转型。
Rect calcTriBBox(const std::array<Vec2i, 3>& tri_subpixel, const Rect& viewport) {
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

/// @brief 边函数计算
/// @details 正常情况下是 cross2D(E,V), V 作为被测试的边。但那样的话，光栅化判断点是否在三角形内部的逻辑就会变成
///          if(E01 <= 0 && E12 <= 0 && E20 <= 0)，这个相比位运算将帧时间的开销增加了将近 10 ms，并且这个逻
///          辑难以写为简单的位运算形式。
/// @note 注意，若交换 V 和 E 的位置，边函数的推导会变化：A 和 B 都要取负。
i64 edgeEquation(const Vec2i& edge_start, const Vec2i& edge_end, const Vec2i& test_point) {
  Vec2i V = test_point - edge_start; // (x-x0, y-y0).
  Vec2i E = edge_end   - edge_start; // (x1-x0, y1-y0).
  return Math::cross2D(V, E);        // (x-x0)(y1-y0)-(x1-x0)(y-y0)
}

/// @brief Top-Left Rule 的偏置计算。
/// @param dx    待测试向量的两端 x 增量；
/// @param dy    待测试向量的两端 y 增量；
/// @details     1. 若像素落在两个三角形共享的边上，则让拥有 Top-Left 边的三角形对其着色；
///              2. 顶边的判定：其 y 增量为 0（水平），且方向向右（左手系，顺时针）；
///                 左边的判定：其 y 增量为正。
/// @return 正好落在三角形边上的屏幕坐标的边函数为零，在增量更新的里层判断会通过并被着色。所以判断三条边
///         哪一个不是 Top-Left 型的，返回一个负偏置使该坐标对应边的 Equation 为负，进而无法着色。
i32 topLeftBias(i32 dx, i32 dy) noexcept {
  bool is_top_left = (dy == 0 && dx > 0) || dy > 0;
  return is_top_left ? 0 : -1;
};

FSPayload interpVaryings(const VaryingLayout& varying_layout,
                         const std::array<Varying, 3>& varyings,
                         const Vec3f& weights,
                         const Vec3f& w_invs,
                         bool  enable_interp_mode = false) {
  // 优化的透视矫正插值路径，但测试过与无优化路径开销毫无差异，可能是 -O3 和 LTO 自动完成了优化；
  // 目前所有插值需求都可以直接走透视插值矫正路径（只需要线性插值的在顶点着色器里将 w 设为 1 即退化），暂时无限期停用另一条判
  // 断插值模式的路径。
  FSPayload fs_payload;
  Interp::PerspWeights pw = Interp::PerspWeights::Get(weights, w_invs);
  if(!enable_interp_mode) {
    // 退化情况直接取第一个顶点的 varying.
    if(pw.w_interp > 0) 
      for(u32 i = 0; i < varying_layout.scalar_cnt; ++i)
        fs_payload.scalars_interp[i]
        = Interp::interpolate(varyings[0].scalars[i], varyings[1].scalars[i], varyings[2].scalars[i], pw);
    else [[unlikely]]
      fs_payload.scalars_interp = varyings[0].scalars;
  } else [[unlikely]] {
    // 当前 scalar 级判断与 field 级开销并无二致。
    for(const auto& field: varying_layout.fields)
    for(u32 i = field.offset; i < field.offset + field.scalar_cnt; ++i)
      if(field.interp_mode == InterpMode::Perspective)
        fs_payload.scalars_interp[i] =
        Interp::interpolate(varyings[0].scalars[i],
                          varyings[1].scalars[i],
                          varyings[2].scalars[i], pw);
      else if(field.interp_mode == InterpMode::NoPerspective)
        fs_payload.scalars_interp[i] =
        Interp::interpolate(varyings[0].scalars[i],
                          varyings[1].scalars[i],
                          varyings[2].scalars[i], weights);
      else
        // 总是拷贝 ClipPolygon 三角划分所得三角形的末顶点属性。
        fs_payload.scalars_interp[i] = varyings[2].scalars[i];
  }
  return fs_payload;
}

} // 匿名命名空间，控制仅单文件可见
} // namespce Medium


namespace Medium::Pipeline {

void executeCommands(const std::vector<RenderCommand>& cmds,
                     const ShaderBindCtx& shader_ctx,
                     const RenderPassCtx& pass_ctx,
                     VariantShader&       shader)
{
  std::visit([&](auto& active_shader) {
    for(const auto& cmd: cmds) {
      active_shader.bind(shader_ctx, cmd.mat_world, cmd.material);

      DrawView draw_view {
        .vertices      = cmd.mesh->vertices,
        .indices       = cmd.mesh->indices,
        .index_offset  = cmd.submesh->index_offset,
        .index_count   = cmd.submesh->index_count,
        .vertex_offset = 0
      };
      
      // 结构化绑定：const auto& 修饰的是 pass_ctx, auto 会推导为 RenderPassCtx. 
      // 故 const auto& 等价于 const RenderPassCtx&.
      const auto& [render_state, viewport, buffer, render_status] = pass_ctx;
      draw(draw_view, render_state, viewport, buffer, shader, render_status);
    }
  }, shader);
}

void draw(const DrawView&    draw_view,
          const RenderState& render_state,
          const RectF&       viewport,
          FrameBuffer&       buffer,
          VariantShader&     shader,
          RenderStat*        render_status)
{
  std::visit([&](auto& active_shader){
    
  const auto& [vertices, indices, index_offset, index_count, vertex_offset] = draw_view;

  for(u32 tri_cnt = index_count / 3, i = 0; i < tri_cnt; ++i) {
    // 每 3 个紧邻的顶点索引为一个三角形。
    u32 tri_idx = index_offset + (i * 3);

    // 将当前三角形作为裁剪输入，运行顶点着色器 World Space -> Clip Space，并获取待插值的 varying.
    ClipPolygon clip_poly;

    for(u32 j = 0; j < 3; ++j, ++clip_poly.num_verts) {
      u32 vert_idx = indices[tri_idx + j];
      
      VSOut vs_out = active_shader.vertex(vertices[vertex_offset + vert_idx]);
      
      clip_poly.verts[j].pos     = vs_out.pos_clip;
      clip_poly.verts[j].varying = vs_out.varying;
    }

    if(!clip(clip_poly, active_shader.varyingScalarCount())) continue;

    clipToNDC(clip_poly);

    // 扇形划分：n 个顶点的凸多边形可分出 n - 2 个三角形
    for(u32 l = 1; l + 1 < clip_poly.num_verts; ++l) {
      std::array<Vec4f, 3> triangle = {
        clip_poly.verts[0].pos, clip_poly.verts[l].pos, clip_poly.verts[l + 1].pos
      };
      std::array<Varying, 3> varyings {
        clip_poly.verts[0].varying, clip_poly.verts[l].varying, clip_poly.verts[l + 1].varying
      };
      // Culling: NDC 空间和屏幕空间的剔除效果一致，所以在视口变换前先做剔除，免得白做一次视口变换；有向面积
      // 为正说明三角形是逆时针的，这在左手系中是背对相机的；
      // - 启用 CullOff   时不会过滤任何三角形；
      // - 启用 CullBack  时只会放行 CW  三角形；
      // - 启用 CullFront 时只会放行 CCW 三角形； 
      bool CW = orient2D(triangle[0], triangle[1], triangle[2]) <= 0;
      if(render_state.cull_mode == CullMode::CullBack  && !CW) continue;
      if(render_state.cull_mode == CullMode::CullFront &&  CW) continue;
      // 若被放行的是 CCW 三角形，说明当前为 CullFront 或 CullOff. 下方的边函数逻辑是专为 CW 三角形优化的，
      // 所以将三角形绕序改为 CW 即可：
      if(!CW) { 
        std::swap(triangle[1], triangle[2]);
        std::swap(varyings[1], varyings[2]);
      }
      // 视口变换 NDC -> Screen Space; 顶点的 x,y 由 [-1.0,1.0] 映射到 [0.0, width], [0.0, height].
      for(auto& vert: triangle) NDCToViewport(vert, viewport);
      rasterize(triangle, varyings, active_shader.varyingLayout(), render_state, viewport, buffer, shader, render_status);
    }
  } // for(i)
  }, shader); // std::visit
} // draw

// 边函数法，Edge Equation.
// E(x,y) = (x-x0)(y1-y0) - (y-y0)(x1-x0)
//        = (y1-y0)x - (x1-x0)y + y0(x1-x0) - x0(y1-y0)
//        = Ax + By + C.
// 故当 E(x+1,y) 有 Delta E = A = y1-y0；当 E(x,y+1) 有 Delta E = B = -(x1-x0) = x0-x1.
// 注：当应用了次像素，边函数就变为了 E(x*scale, y*scale) = (A*scale)x + (B*scale)y + C.
void rasterize(const std::array<Vec4f, 3>&   triangle,
               const std::array<Varying, 3>& varyings,
               const VaryingLayout& varying_layout,
               const RenderState&   render_state,
               const RectF&   viewport,
               FrameBuffer&   buffer,
               VariantShader& shader,
               RenderStat*    render_status)
{
  std::array<Vec2i, 3> tri_subpixel; // 将传入三角形的浮点屏幕坐标提升为次像素定点数；
  std::array<f32  , 3> vert_depth;   // 保存三角形顶点的深度值 Zndc.

  // 像素空间 -> 次像素空间
  for(size_t i = 0; i < 3; ++i) {
    tri_subpixel[i].x() = Subpixel::fromFloat(triangle[i].x());
    tri_subpixel[i].y() = Subpixel::fromFloat(triangle[i].y());
    vert_depth[i]       = triangle[i].z();
  }

  Rect bbox_tri = calcTriBBox(tri_subpixel, viewport);
  if(!bbox_tri.valid()) return; // 若三角形完全在视区外，直接丢弃

  // 若 RenderState 带的 scissor 非法，说明未启用 scissor test;
  // 若启用，则求其与当前三角包围盒的交集，若无交集，直接放弃接下来的着色。
  if(const Rect& scissor = render_state.scissor; scissor.valid())
    if(bbox_tri = scissor.intersection(bbox_tri); !bbox_tri.valid())
      return;

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

  if(render_status) render_status->statistics.visible_triangles++;

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
  for(i32 y = bbox_tri.min().y(); y <= bbox_tri.max().y(); ++y) {
    // 当前像素的 Equation 值，注意同样要采用 i64.
    i64 E01 = E01_row; // 三角形 V0V1P 面积的二倍
    i64 E12 = E12_row; // 三角形 V1V2P 面积的二倍
    i64 E20 = E20_row; // 三角形 V2V0P 面积的二倍
    for(i32 x = bbox_tri.min().x(); x <= bbox_tri.max().x(); ++x) {
      // 任意一个为负，其符号位为 1，OR 后整体为负，说明在三角形外。
      if((E01 | E12 | E20) >= 0) {
        // Equation 值需临时减去偏置，重心坐标需要精确的值（V0 对应对边 E12，其它同理）。
        Vec3f weights{
          static_cast<f32>(E12 - bias12) * area_total_inv,
          static_cast<f32>(E20 - bias20) * area_total_inv,
          static_cast<f32>(E01 - bias01) * area_total_inv
        };
        // Zndc 已是 A+B/z 的形式，在屏幕空间是线性的，无需透视矫正插值。
        f32 z_interp = Interp::interpolate(vert_depth[0], vert_depth[1], vert_depth[2], weights);
        // Early-Z.
        size_t idx = buffer.getIndex(x, y);
        // 当深度测试关闭时，不能做 continue 的处理，否则会跳过 E(x+1,Y) 的推进。
        bool pass_depth_test = !render_state.enable_depth_test || buffer.depthTest(idx, z_interp);
        
        if(pass_depth_test) {
          Vec3f w_invs{varyings[0].w_inv, varyings[1].w_inv, varyings[2].w_inv};
        
          FSPayload fs_payload = interpVaryings(varying_layout, varyings, weights, w_invs, false);

          bool discard = active_shader.fragment(fs_payload); // Alpha test.  
          
          if(!discard) {
            if(render_state.enable_depth_write)
              buffer.writeDepth(idx, z_interp);
            if(render_state.enable_color_write) {
              if(render_state.blend_mode == BlendMode::Replace) {
                if(render_state.color_write_target == ColorWriteTarget::HDR)
                  buffer.writeHDRColor(idx, fs_payload.frag_color);
                else
                  buffer.writePresentColor(idx, fs_payload.frag_color);
              }
              else {
                if(render_state.color_write_target == ColorWriteTarget::HDR)
                  buffer.writeHDRColorPremult(idx, fs_payload.frag_color.premultiplied());
                else
                  buffer.writePresentColorPremult(idx, fs_payload.frag_color.premultiplied());
              }
              if(render_status) render_status->statistics.written_pixels++;
            }
          }
          
        } // if(pass_depth_test) 
      } // if((E01 | E12 | E20) >= 0)
      // E(X+1,Y) 方向推进
      E01 += A01;
      E12 += A12;
      E20 += A20;
    }
    // 换行，E(X,Y+1) 方向推进
    E01_row += B01;
    E12_row += B12;
    E20_row += B20;
  } 
  }, shader);  // std::visit

} // rasterize

} // namespace Medium::Pipeline
