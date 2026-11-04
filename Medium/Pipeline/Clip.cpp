///
/// @file   Clip.cpp
/// @author Garfield949
/// @date   2026/6/30 15:15
/// @brief  
///
#include "Clip.h"

namespace Medium {
  
namespace {

constexpr std::array kClipPlanes = {
  Near, Left, Right, Top, Bottom, Far
};

/// @brief 计算顶点在哪些裁剪面之外。
int calcOutcode(const Vec4f& v) noexcept {
  int code = None;
  if(v.x() < -v.w()) code |= Left;
  if(v.x() >  v.w()) code |= Right;
  if(v.y() >  v.w()) code |= Top;
  if(v.y() < -v.w()) code |= Bottom;
  if(v.z() >  v.w()) code |= Near;
  if(v.z() <  0.0f)  code |= Far;
  return code;
}

/// @brief 计算顶点到目标裁剪面的有符号距离。
f32 signedDist(const Vec4f& v, const ClipPlane plane) noexcept {
  switch(plane) {
    case Left:   return v.w() + v.x();
    case Right:  return v.w() - v.x();
    case Top:    return v.w() - v.y();
    case Bottom: return v.w() + v.y();
    case Near:   return v.w() - v.z();
    case Far:    return v.z();
    default: return 0.0f;
  }
}

/// @brief 输入一个多边形，输出其被目标裁剪面裁剪后的多边形。
ClipPolygon clipByPlane(const ClipPolygon& poly_in, const ClipPlane plane, i32 varying_scalar_count) noexcept {
  ClipPolygon poly_out;

  // 遍历输入多边形的所有边，由 head -> tail 定义
  for(i32 i = 0; i < poly_in.num_verts; ++i) {
    // 当 i = num_verts - 1, (i + 1) % num_verts = 0，尾首完成闭环
    const ClipVertex* v_head = &poly_in.verts[i];
    const ClipVertex* v_tail = &poly_in.verts[(i + 1) % poly_in.num_verts];

    f32 d_head = signedDist(v_head->pos, plane);
    f32 d_tail = signedDist(v_tail->pos, plane);

    bool inside_head = d_head >= 0.0f;
    bool inside_tail = d_tail >= 0.0f;

    // 当前顶点若在裁剪面内则直接加入多边形
    if(inside_head)
      poly_out.verts[poly_out.num_verts++] = *v_head;

    // 若当前、下一个顶点不在同侧，说明此边与裁剪面有交点
    if(inside_head != inside_tail)
      poly_out.verts[poly_out.num_verts++]
        = ClipVertex::Lerp(*v_head, *v_tail, d_head / (d_head - d_tail), varying_scalar_count);
  }

  return poly_out;
}

} // ===---------- 匿名命名空间，控制仅单文件可见 ----------===

// 不对插值过程中的 normal 和 tangent 进行归一化，因为 fragment 阶段会归一化。
ClipVertex ClipVertex::Lerp(const ClipVertex& a, const ClipVertex& b, f32 t, i32 varying_scalar_count) noexcept {
  ClipVertex out;
  out.pos = Math::lerp(a.pos, b.pos, t);

  for(i32 i = 0; i < varying_scalar_count; ++i)
    out.varying.scalars[i] = Math::lerp(a.varying.scalars[i], b.varying.scalars[i], t);

  return out;
}

bool clipTriangle(ClipPolygon& tri, i32 varying_scalar_count) noexcept {
  int code0 = calcOutcode(tri.verts[0].pos);
  int code1 = calcOutcode(tri.verts[1].pos);
  int code2 = calcOutcode(tri.verts[2].pos);

  // 三个顶点均在某裁剪面外侧，不可见直接丢弃
  if((code0 & code1 & code2) != None) return false;

  int union_code = code0 | code1 | code2;

  // 三个顶点都不在任意一个裁剪面外部，则面片在视椎内，无需裁剪
  if(union_code == None) return true;

  // 剩下的情况：三角形可能有一条边跨越了某个裁剪面。
  // 依次被六个面裁剪，若某次被裁到只剩不到三个顶点，说明三角形不在视椎体内部:
  // 1. 只有一个顶点：多边形只有一个顶点落在裁剪面上，其它顶点都在视椎外部；
  // 2. 只有两个顶点：多边形有一条边落在裁剪面上，其它顶点在视椎外部。
  for(const auto& plane: kClipPlanes) {
    if((union_code & plane) == None) continue;
    tri = clipByPlane(tri, plane, varying_scalar_count);
    if(tri.num_verts < 3) return false;
  }
  return true;
}

bool clipLine(Vec4f& v0, Vec4f& v1) noexcept {
  Vec4f head = v0;
  Vec4f tail = v1;
  Vec4f head_to_tail = tail - head;

  float t_enter = 0.0f;
  float t_leave = 1.0f;

  auto clipPlane = [&](float d_head, float d_tail) -> bool {
    // 想象为一维：裁剪面为零点，面内属于正半轴，面外属于负半轴。只要 head 在 tail 的右边 d 就一定为正。
    float d = d_head - d_tail;
    // 俩端点到裁剪面的距离一样 -> 平行于裁剪面 -> 只需检查其中一个是否在裁剪面内。
    if(d == 0.0f) return d_head >= 0.0f;
    // 端点跨越裁剪面的情形，举例：
    // - d_head = -1, d_tail =  3, d = -4, t = 1/4 (从面外走向面内)
    // - d_head =  2, d_tail = -3, d =  5, t = 2/5 (从面内走向面外)
    // 可观察到：
    // - d < 0 代表 head 从面外走向面内的 tail; d > 0 代表 head 从面内走向面外的 tail.
    // - t 的值域：[0,1].
    //
    // 端点在裁剪面同侧的情形，举例：
    // - d_head =  1, d_tail =  3, d = -2, t = -1/2 (均在面内)
    // - d_head =  5, d_tail =  2, d =  3, t =  5/3 (均在面内)
    // - d_head = -1, d_tail = -2, d =  1, t = -1   (均在面外)
    // - d_head = -4, d_tail = -1, d = -3, t =  4/3 (均在面外)
    // 可观察到：t < 0 或 t > 1.
    float t = d_head / d;
    // d_head > d_tail -> 线段从面内走向面外 -> 交点是离开点 (leave)
    if(d > 0.0f && t < t_leave) t_leave = t;
    // d_head < d_tail -> 线段从面外走向面内 -> 交点是进入点 (enter)
    if(d < 0.0f && t > t_enter) t_enter = t;
    // 如果是均在面内，则 t_enter 与 t_leave 均不会被更新；
    // 如果是均在面外，则会导致 t_enter > t_leave.
    return t_enter <= t_leave;
  };

  // 被六个面轮流裁一次。若完全在其中一个裁剪面的外部，说明不在视椎体内，返回 false.
  for(const auto& plane : kClipPlanes)
    if(!clipPlane(signedDist(head, plane), signedDist(tail, plane)))
      return false;

  // 根据最终合法的 t 裁剪区间更新端点坐标
  v0 = head + head_to_tail * t_enter;
  v1 = head + head_to_tail * t_leave;

  return true;
}

} // namespace Medium