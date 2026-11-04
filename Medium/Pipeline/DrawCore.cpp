///
/// @file   DrawCore.cpp
/// @author Garfield949
/// @date   2026/3/1 20:57
/// @brief
///
#include "DrawCore.h"

#include <span>

#include "Base/Types.h"
#include "Base/Color.h"
#include "Base/Geometry/Rect.h"
#include "Base/Math/Interpolation.h"

#include "Pipeline/Clip.h"
#include "Pipeline/Subpixel.h"
#include "Pipeline/FrameBuffer.h"
#include "Pipeline/DrawCommand.h"
#include "Pipeline/RenderContext.h"
#include "Pipeline/Shader/ShaderTypes.h"

#include "Resources/Mesh.h"

namespace Medium::Pipeline {

namespace {
// 透视除法 x,y -> [-1,1], reversed-z -> [0,1].
// 正交投影 w 恒为 1，可以不走这个循环，但非热点不用管。
void clipToNDC(ClipPolygon& clip_poly) {
  for(i32 k = 0; k < clip_poly.num_verts; ++k) {
    Vec4f& pos = clip_poly.verts[k].pos;
    const f32 w_inv = 1.0f / pos.w();
    pos.x() *= w_inv;
    pos.y() *= w_inv;
    pos.z() *= w_inv;
  }
}

/// @brief   视口变换。
/// @details 接收一个 NDC 坐标，将其 x,y 分量映射到以给定视口。
void NDCToViewport(Vec4f& vert, const RectF& viewport) {
  vert.x() = viewport.x + 0.5f * viewport.w * (vert.x() + 1.0f);
  vert.y() = viewport.y + 0.5f * viewport.h * (vert.y() + 1.0f);
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
  const Vec2f v1 = b.head<2>() - a.head<2>();
  const Vec2f v2 = c.head<2>() - a.head<2>();
  return Math::cross2D(v1, v2);
}

/// @brief 计算三角形的屏幕包围盒。
/// @details Viewport 的作用是进行边界截断，会从 rasterizeTriangle 传入一个 RectF 自动隐式转型。
Rect calcTriBBox(const std::array<Vec2i, 3>& tri_subpixel, const Rect& viewport) {
  // 获取次像素空间的包围盒边界。
  const i32 x_min_sub = std::min({tri_subpixel[0].x(), tri_subpixel[1].x(), tri_subpixel[2].x()});
  const i32 x_max_sub = std::max({tri_subpixel[0].x(), tri_subpixel[1].x(), tri_subpixel[2].x()});
  const i32 y_min_sub = std::min({tri_subpixel[0].y(), tri_subpixel[1].y(), tri_subpixel[2].y()});
  const i32 y_max_sub = std::max({tri_subpixel[0].y(), tri_subpixel[1].y(), tri_subpixel[2].y()});
  // 将边界从次像素空间转回像素空间。进行安全的取整策略：宁可多框一些像素，避免漏掉边缘像素。
  // - Floor   行为：单纯右移，等价于除以 16. 使得在 [160, 176) 内的值取整结果为 10;
  // - Ceiling 行为：加一个 bias 再右移。这里加 15，使得在 (160, 176] 内的值取整结果为 11;
  // - 再针对 viewport 的边界值截断一下。
  const i32 bias  = Subpixel::kScale - 1;
  const i32 x_min = std::max(viewport.x, x_min_sub >> Subpixel::kBits);
  const i32 y_min = std::max(viewport.y, y_min_sub >> Subpixel::kBits);
  const i32 x_max = std::min(viewport.x + viewport.w - 1, (x_max_sub + bias) >> Subpixel::kBits);
  const i32 y_max = std::min(viewport.y + viewport.h - 1, (y_max_sub + bias) >> Subpixel::kBits);
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
  const Vec2i V = test_point - edge_start; // (x-x0, y-y0).
  const Vec2i E = edge_end   - edge_start; // (x1-x0, y1-y0).
  return Math::cross2D(V, E);              // (x-x0)(y1-y0)-(x1-x0)(y-y0)
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
  const bool is_top_left = (dy == 0 && dx > 0) || dy > 0;
  return is_top_left ? 0 : -1;
}

/// @struct UVGradientParams
/// @brirf 预计算 UV 屏幕导数所需的参数。
struct UVGradientParams {
  explicit UVGradientParams(const std::array<Varying, 3>& varyings,
                            const Vec3f& w_invs,
                            const Vec3f& weights_dx,
                            const Vec3f& weights_dy) {
    // 计算 UV 屏幕导数。回忆透视矫正插值的公式：
    // 假设有三个顶点属性 a0,a1,a2, 重心坐标为 λ0,λ1,λ2, 裁剪空间 w 为 w0,w1,w2, 则有
    //   a = [a0 * (λ0/w0) + a1 * (λ1/w1) + a2 * (λ2/w2)] / [λ0/w0 + λ1/w1 + λ2/w2].
    // 以求 U 对 x 的偏导数 Ux 为例：
    // 根据透视矫正插值公式，设 U = N/D, 其中 N = u0 * (λ0/w0) + u1 * (λ1/w1) + u2 * (λ2/w2), D = λ0/w0 + λ1/w1 + λ2/w2.
    // 根据商法则, dU/dx = [Nx * D - N * Dx] / D^2 = [Nx - U * Dx] / D.
    // 观察 N 可知 ui/wi 是常数，因此 dN/dx = dλ0/dx * u0/w0 + dλ1/dx * u1/w1 + dλ2/dx * u2/w2.
    // 至于 D 求导就比较简单，dD/dx = dλ0/dx / w0 + dλ1/dx / w1 + dλ2/dx / w2.
    // Uy, Vx, Vy 同理；下面的计算逻辑用到了一些中间结果。
    const Vec2f uv0_over_w = Vec2f{varyings[0].scalars[kOffsetUV], varyings[0].scalars[kOffsetUV + 1]} * w_invs[0];
    const Vec2f uv1_over_w = Vec2f{varyings[1].scalars[kOffsetUV], varyings[1].scalars[kOffsetUV + 1]} * w_invs[1];
    const Vec2f uv2_over_w = Vec2f{varyings[2].scalars[kOffsetUV], varyings[2].scalars[kOffsetUV + 1]} * w_invs[2];

    N_dx = uv0_over_w * weights_dx[0] + uv1_over_w * weights_dx[1] + uv2_over_w * weights_dx[2];
    N_dy = uv0_over_w * weights_dy[0] + uv1_over_w * weights_dy[1] + uv2_over_w * weights_dy[2];

    D_dx = weights_dx.dot(w_invs);
    D_dy = weights_dy.dot(w_invs);
  }
  Vec2f N_dx, N_dy;
  f32   D_dx, D_dy;
};

/// @brief 插值、计算 UV 屏幕导数。
FSIn computeFSIn(const std::array<Varying, 3>& varyings,
                 const i32 varying_scalar_count,
                 const Vec3f& weights,
                 const Vec3f& w_invs,
                 const UVGradientParams& params) {
  FSIn fs_in;

  // 优化的透视矫正插值路径，但测试过与无优化路径开销毫无差异，可能是 -O3 和 LTO 自动完成了优化；
  // 目前所有插值需求都可以直接走透视插值矫正路径（只需要线性插值的在顶点着色器里将 w 设为 1 即退化）
  const Interp::PerspWeights pw = Interp::PerspWeights::Get(weights, w_invs);

  // 透视矫正插值
  for(i32 i = 0; i < varying_scalar_count; ++i)
      fs_in.scalars_interp[i]
      = Interp::interpolate(varyings[0].scalars[i], varyings[1].scalars[i], varyings[2].scalars[i], pw);
  
  // u = N/D, 这里直接用插值完成的结果省去计算。
  const Vec2f uv = fs_in.getVec2f(0);
  
  // pw.w_interp 即 1 / D.
  fs_in.duv_dx = (params.N_dx - uv * params.D_dx) * pw.w_interp;
  fs_in.duv_dy = (params.N_dy - uv * params.D_dy) * pw.w_interp;

  return fs_in;
}

void bresenham(int x0, int y0, f32 z0,
               int x1, int y1, f32 z1,
               const RenderState& state,
               const LinearColor& color,
               FrameBuffer&       buffer,
               RenderStatus*      render_status) { 
  // 确保x为最大位移方向
  bool steep = false; 
  if(std::abs(x0 - x1) < std::abs(y0 - y1)) { 
    std::swap(x0, y0); 
    std::swap(x1, y1);
    steep = true; 
  }
  
  // 确保计算方向为正向
  if(x0 > x1) {
    std::swap(x0, x1);
    std::swap(y0, y1);
    std::swap(z0, z1);
  }

  i32 dX = x1 - x0;
  // dX 为 0 直接绘制单点然后返回，否则下方的插值会导致除零。
  if (dX == 0) {
    size_t idx = !steep ? buffer.index(x0, y0) : buffer.index(y0, x0);
    if(buffer.depthTest(idx, z0, state.depth_cmp)) {
      buffer.writeDepth(idx, z0);
      buffer.writeHDRColor(idx, color);
      if(render_status) ++render_status->statistics.written_pixels;
    }
    return;
  }

  i32 dY = std::abs(y1 - y0);
  i32 delta = dX - 2 * dY;

  i32 dStepUp = 2 * (dX - dY);
  i32 dStepDown = -2 * dY;

  i32 x = x0;
  i32 y = y0;

  for(i32 i = x; i <= x1; i++) {
    size_t idx = !steep ? buffer.index(i, y) : buffer.index(y, i);
    f32 z = Math::lerp(z0, z1, static_cast<f32>(i - x0) / static_cast<f32>(dX));
    if(buffer.depthTest(idx, z, state.depth_cmp)) {
      buffer.writeDepth(idx, z);
      buffer.writeHDRColor(idx, color);
      if(render_status) ++render_status->statistics.written_pixels;
    }
    if(delta < 0) {
        y += y0 < y1 ? 1 : -1;
        delta += dStepUp;
    } else {
        delta += dStepDown;
    }
  }
}

/// @brief 做最终片段写入工作。
void writeFragment(size_t             frag_index, 
                   f32                z_interp,
                   const RenderState& state,
                   const LinearColor& color,
                   FrameBuffer&       buffer,
                   RenderStatus*      status) {
  if(state.enable_depth_write) buffer.writeDepth(frag_index, z_interp);
  if(state.enable_color_write) {
    if(state.blend_mode == BlendMode::Replace) {
      if(state.color_write_target == ColorWriteTarget::HDR)
        buffer.writeHDRColor(frag_index, color);
      else
        buffer.writePresentColor(frag_index, color);
    }
    else {
      if(state.color_write_target == ColorWriteTarget::HDR)
        buffer.writeHDRColorPremult(frag_index, color.premultiplied());
      else
        buffer.writePresentColorPremult(frag_index, color.premultiplied());
    }
    if(status) status->statistics.written_pixels++;
  }
}

// 边函数法，Edge Equation.
// E(x,y) = (x-x0)(y1-y0) - (y-y0)(x1-x0)
//        = (y1-y0)x - (x1-x0)y + y0(x1-x0) - x0(y1-y0)
//        = Ax + By + C.
// 故当 E(x+1,y) 有 Delta E = A = y1-y0；当 E(x,y+1) 有 Delta E = B = -(x1-x0) = x0-x1.
// 注：当应用了次像素，边函数就变为了 E(x*scale, y*scale) = (A*scale)x + (B*scale)y + C.
void rasterizeTriangle(const std::array<Vec4f, 3>&   triangle,
                       const std::array<Varying, 3>& varyings,
                       const RenderState&            render_state,
                       const RectF&                  viewport,
                       ShaderVariant&                shader,
                       FrameBuffer&                  buffer,
                       RenderStatus*                 render_status)
{
  std::array<Vec2i, 3> tri_fixed; // 将传入三角形的浮点屏幕坐标提升为次像素定点数；
  std::array<f32  , 3> tri_depth; // 保存三角形顶点的深度值 Zndc.

  // 像素空间 -> 次像素空间
  for(size_t i = 0; i < 3; ++i) {
    tri_fixed[i].x() = Subpixel::fromF32(triangle[i].x());
    tri_fixed[i].y() = Subpixel::fromF32(triangle[i].y());
    tri_depth[i]     = triangle[i].z();
  }

  // 注：计算得到的 bbox_tri 属于像素空间（非subpixel）
  Rect bbox_tri = calcTriBBox(tri_fixed, viewport);
  if(!bbox_tri.valid()) return; // 若三角形完全在视区外，直接丢弃

  // 若 RenderState 带的 scissor 非法，说明未启用 scissor test;
  // 若启用，则求其与当前三角包围盒的交集，若无交集则放弃接下来的着色。
  if(const Rect& scissor = render_state.scissor; scissor.valid())
    if(bbox_tri = scissor.intersection(bbox_tri); !bbox_tri.valid())
      return;

  // 中间量语义化
  const Vec2i delta01{tri_fixed[1].x() - tri_fixed[0].x(),  tri_fixed[1].y() - tri_fixed[0].y()};
  const Vec2i delta12{tri_fixed[2].x() - tri_fixed[1].x(),  tri_fixed[2].y() - tri_fixed[1].y()};
  const Vec2i delta20{tri_fixed[0].x() - tri_fixed[2].x(),  tri_fixed[0].y() - tri_fixed[2].y()};

  // A 对应边函数 E(X+1,Y) 时的增量；同理，B 对应 E(X,Y+1) 时的增量。
  // 在次像素空间中，每推近一个屏幕像素，相当于在次像素空间增加 16.
  const i32 A01 = Subpixel::kScale * delta01.y(),  B01 = -Subpixel::kScale * delta01.x();
  const i32 A12 = Subpixel::kScale * delta12.y(),  B12 = -Subpixel::kScale * delta12.x();
  const i32 A20 = Subpixel::kScale * delta20.y(),  B20 = -Subpixel::kScale * delta20.x();

  // 从包围盒左下角的坐标开始测试、着色，一行一行往上；加一个偏置将采样点移到像素中心，这里 array 方法将向量转为数组以进行逐分量加法。
  // Subpixel::kHalf = 8.
  const Vec2i p_start = (bbox_tri.min() * Subpixel::kScale).array() + Subpixel::kHalf;

  // 开头获取首坐标对应三条边的 Equation，后续的坐标只需在此基础上加上推导好的增量即可。
  i64 E01_row = edgeEquation(tri_fixed[0],  tri_fixed[1], p_start);
  i64 E12_row = edgeEquation(tri_fixed[1],  tri_fixed[2], p_start);
  i64 E20_row = edgeEquation(tri_fixed[2],  tri_fixed[0], p_start);

  const i64 area_total = E01_row + E12_row + E20_row;
  if(area_total == 0) return; // 丢弃退化三角形

  if(render_status) render_status->statistics.visible_polygons++;

  const f32 area_total_inv = 1.0f / static_cast<f32>(area_total);

  // 已知 E(x,y) = Ax + By + C --> E(x+1,y) = E(x,y) + A
  // 而重心坐标 λ(x,y) = E(x,y) / total_area
  // 因此对等式 E(x+1,y) = E(x,y) + A 两边同时除以 total_area 得到
  // E(x+1,y) / total_area = λ(x+1,y) = E(x,y) / total_area + A / total_area
  //                                  = λ(x,y) + A / total_area
  // 所以 A / total_area 的数学意义为：当屏幕像素沿 x 轴变化一格时，对应三角形顶点权重的变化量 dλ / dx.
  const Vec3f weights_dx = Vec3f{
    static_cast<f32>(A12), static_cast<f32>(A20), static_cast<f32>(A01)
  } * area_total_inv;

  // 同理 B / total_area 的数学意义为：当屏幕像素沿 y 轴变化一格时，对应三角形顶点权重的变化量 dλ / dy.
  const Vec3f weights_dy = Vec3f{
    static_cast<f32>(B12), static_cast<f32>(B20), static_cast<f32>(B01)
  } * area_total_inv;

  const Vec3f w_invs{1.0f / triangle[0].w(), 1.0f / triangle[1].w(), 1.0f / triangle[2].w()};

  const UVGradientParams uv_gradient_params{varyings, w_invs, weights_dx, weights_dy};

  // 缓存 Top-Left 的偏置值
  const i32 bias01 = topLeftBias(delta01.x(),  delta01.y());
  const i32 bias12 = topLeftBias(delta12.x(),  delta12.y());
  const i32 bias20 = topLeftBias(delta20.x(),  delta20.y());

  // Top-Left Rule 的偏置在计算完面积后再加，否则面积计算会受影响
  E01_row += bias01;
  E12_row += bias12;
  E20_row += bias20;
 
  // 三角形层面做 dispatch
  std::visit([&](auto& active_shader) {
  // 遍历包围盒，边函数增量更新。
  for(i32 y = bbox_tri.min().y(); y <= bbox_tri.max().y(); ++y) {
    // 当前像素的 Equation 值，注意同样要采用 i64.
    // 注：E*_row 保存当前行首的边函数值，E* 作为副本在行内向右递增。换行时，E*_row 加上对应的 B 得到新行
    // 首值，再用它初始化 E*. 若直接在行内递增 E*_row, 行末仅加 B 会保留整行的水平偏移，无法回到下一行行首。
    i64 E01 = E01_row; // 三角形 V0V1P 面积的二倍
    i64 E12 = E12_row; // 三角形 V1V2P 面积的二倍
    i64 E20 = E20_row; // 三角形 V2V0P 面积的二倍
    for(i32 x = bbox_tri.min().x(); x <= bbox_tri.max().x(); ++x) {
      // 任意一个为负，其符号位为 1，OR 后整体为负，说明在三角形外。
      if((E01 | E12 | E20) >= 0) {
        // Equation 值需临时减去偏置，重心坐标需要精确的值；
        // V0 对应对边 E12: P 越靠近 V0, 则 V0 对向的三角形 V1V2P 一定越大，两者呈正相关。
        const Vec3f weights{
          static_cast<f32>(E12 - bias12) * area_total_inv,
          static_cast<f32>(E20 - bias20) * area_total_inv,
          static_cast<f32>(E01 - bias01) * area_total_inv
        };
        
        // Zndc 已是 A+B/z 的形式，在屏幕空间是线性的，无需透视矫正插值。
        const f32 z_interp = Interp::interpolate(tri_depth[0], tri_depth[1], tri_depth[2], weights);
        
        // Early-Z.
        const size_t pixel_idx = buffer.index(x, y);
        
        // 当深度测试关闭时不能直接 continue，否则会跳过 E(x+1,Y) 的推进。
        const bool pass_depth_test 
          = !render_state.enable_depth_test || buffer.depthTest(pixel_idx, z_interp, render_state.depth_cmp);
        
        if(pass_depth_test) {
          FSIn fs_in = computeFSIn(varyings, active_shader.kVaryingScalarCount, weights, w_invs, uv_gradient_params);
          fs_in.pixel_index = pixel_idx;

          LinearColor frag_color;
          if(!active_shader.fragment(fs_in, frag_color)) 
            writeFragment(pixel_idx, z_interp, render_state, frag_color, buffer, render_status);
        }
      }
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
}

} // ===---------- 匿名命名空间，控制仅单文件可见 ----------===

void executeDrawCommands(std::span<const DrawCommand> cmds,
                         const ShaderBindContext&     shader_ctx,
                         const RenderPassContext&     pass_ctx,
                         ShaderVariant&               shader)
{
  std::visit([&](auto& active_shader) {
  for(const auto& cmd: cmds) {
    active_shader.bind(shader_ctx, cmd.mat_world, cmd.material);

    IndexedDrawView draw_view {
      .vertices      = cmd.mesh->vertices,
      .indices       = cmd.mesh->indices,
      .index_offset  = cmd.submesh->index_offset,
      .index_count   = cmd.submesh->index_count
    };
    
    // 结构化绑定：const auto& 修饰的是 pass_ctx, auto 会推导为 RenderPassContext. 
    // 故 const auto& 等价于 const RenderPassContext&.
    const auto& [render_state, viewport, buffer, render_status] = pass_ctx;
    draw(draw_view, render_state, viewport, buffer, shader, render_status);
  }
  }, shader);
}

void draw(const IndexedDrawView& draw_view,
          const RenderState&     render_state,
          const RectF&           viewport,
          FrameBuffer&           buffer,
          ShaderVariant&         shader,
          RenderStatus*          render_status)
{
  std::visit([&](auto& active_shader){
    
  const auto& 
  [vertices, indices, index_offset, index_count, vertex_offset] = draw_view;

  for(size_t tri_cnt = index_count / 3, i = 0; i < tri_cnt; ++i) {
    // 每 3 个紧邻的顶点索引为一个三角形。
    size_t tri_idx = index_offset + i * 3;

    // 将当前三角形作为裁剪输入，运行顶点着色器 World Space -> Clip Space，并获取待插值的 varying.
    ClipPolygon polygon;    
    for(size_t j = 0; j < 3; ++j, ++polygon.num_verts) {
      size_t vert_idx = indices[tri_idx + j];
      
      const VSOut vs_out = active_shader.vertex(vertices[vertex_offset + vert_idx]);
      
      polygon.verts[j].pos     = vs_out.pos_clip;
      polygon.verts[j].varying = vs_out.varying;
    }

    if(!clipTriangle(polygon, active_shader.kVaryingScalarCount)) continue;
    
    clipToNDC(polygon);

    if(render_state.draw_mode == DrawMode::Wireframe) {
      // 线框模式总是 cull 掉背面。
      if(!(orient2D(polygon.verts[0].pos, polygon.verts[1].pos, polygon.verts[2].pos) <= 0))
        continue;

      if(render_status) render_status->statistics.visible_polygons++;

      for(i32 k = 0; k < polygon.num_verts; ++k) {
        Vec4f v0 = polygon.verts[k].pos;
        Vec4f v1 = polygon.verts[(k + 1) % polygon.num_verts].pos;
        rasterizeLine(v0, v1, render_state, viewport, LinearColor::Navy(), buffer, render_status);
      }
      continue;
    }

    // 扇形划分：n 个顶点的凸多边形可分出 n - 2 个三角形
    for(i32 l = 1; l + 1 < polygon.num_verts; ++l) {
      std::array<Vec4f, 3> triangle = {
        polygon.verts[0].pos, polygon.verts[l].pos, polygon.verts[l + 1].pos
      };
      
      std::array<Varying, 3> varyings {
        polygon.verts[0].varying, polygon.verts[l].varying, polygon.verts[l + 1].varying
      };
      // Culling: NDC 空间和屏幕空间的剔除效果一致，所以在视口变换前先做剔除，免得白做一次视口变换；有向面积
      // 为正说明三角形是逆时针的，这在左手系中是背对相机的；
      // - 启用 Off   时不会过滤任何三角形；
      // - 启用 Back  时只会放行 CW  三角形；
      // - 启用 Front 时只会放行 CCW 三角形； 
      const bool CW = orient2D(triangle[0], triangle[1], triangle[2]) <= 0;
      if(render_state.cull_mode == CullMode::Back  && !CW) continue;
      if(render_state.cull_mode == CullMode::Front &&  CW) continue;
      // 若被放行的是 CCW 三角形，说明当前为 Front 或 Off. 下方的边函数逻辑是专为 CW 三角形优化的，
      // 所以将三角形绕序改为 CW 即可：
      if(!CW) { 
        std::swap(triangle[1], triangle[2]);
        std::swap(varyings[1], varyings[2]);
      }
      // 视口变换 NDC -> Screen Space; 顶点的 x,y 由 [-1.0,1.0] 映射到 [0.0, width], [0.0, height].
      for(auto& vert: triangle) NDCToViewport(vert, viewport);

      rasterizeTriangle(triangle, varyings, render_state, viewport, shader, buffer, render_status);
    }
  } // for(i)
  }, shader); // std::visit
} // draw

bool rasterizeLine(Vec4f&             v0,
                   Vec4f&             v1,
                   const RenderState& state,
                   const RectF&       viewport,
                   const LinearColor& color,
                   FrameBuffer&       buffer,
                   RenderStatus*      render_status)
{  
  NDCToViewport(v0, viewport);
  NDCToViewport(v1, viewport);
  
  const f32 x_max = viewport.x + viewport.w - 1;
  const f32 y_max = viewport.y + viewport.h - 1;
  
  const i32 x0 = static_cast<i32>(std::min(std::round(v0.x()), x_max));
  const i32 y0 = static_cast<i32>(std::min(std::round(v0.y()), y_max));
  const i32 x1 = static_cast<i32>(std::min(std::round(v1.x()), x_max));
  const i32 y1 = static_cast<i32>(std::min(std::round(v1.y()), y_max));

  bresenham(x0, y0, v0.z(), x1, y1, v1.z(), state, color, buffer, render_status);

  return true;
}

} // namespace Medium::Pipeline
