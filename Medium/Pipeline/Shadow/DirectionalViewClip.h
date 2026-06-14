///
/// @file   DirectionalViewClip.h
/// @author Garfield949
/// @date   2026/5/26 08:16
/// @brief  Clip.h 的 2D 特化版本
///
#pragma once

#include "Base/Types.h"
#include "Base/Math/Math.h"

namespace Medium {

// 定义裁剪面枚举
enum FrustumSide {
  Left   = 1 << 0,
  Right  = 1 << 1,
  Top    = 1 << 2,
  Bottom = 1 << 3,
  None   = 0
};

inline int calcOutcode(const Vec2f& light_ortho_min,
                       const Vec2f& light_ortho_max,
                       const Vec3f& v) noexcept {
  int code = None;
  if(v.x() < light_ortho_min.x()) code |= Left;
  if(v.x() > light_ortho_max.x()) code |= Right;
  if(v.y() > light_ortho_max.y()) code |= Top;
  if(v.y() < light_ortho_min.y()) code |= Bottom;
  return code;
}

// 三角形初始有 3 个顶点，每次与裁剪面相交最多新产生 1 个顶点；遍历 4 个面便会最多得到 7 个顶点
constexpr size_t kMaxClipVertsNum = 7;
struct ClipPolygon {
  std::array<Vec3f, kMaxClipVertsNum> verts;
  size_t num_verts = 0;
};

inline f32 calcSignedDistance(const Vec2f& light_ortho_min,
                              const Vec2f& light_ortho_max,
                              const Vec3f& v, FrustumSide plane) noexcept {
  switch(plane) {
  case Left:   return v.x() - light_ortho_min.x();
  case Right:  return light_ortho_max.x() - v.x();
  case Top:    return light_ortho_max.y() - v.y();
  case Bottom: return v.y() - light_ortho_min.y();
  default: return 0.0f;
  }
}

inline ClipPolygon clipByPlane(const Vec2f& light_ortho_min,
                               const Vec2f& light_ortho_max,
                               const ClipPolygon& poly_in,
                               const FrustumSide  plane) noexcept {
  ClipPolygon poly_out;

  // 遍历输入多边形的所有边，由 head -> tail 定义
  for(size_t i = 0; i < poly_in.num_verts; ++i) {
    // 当 i = num_verts - 1, (i + 1) % num_verts = 0，尾首完成闭环
    const Vec3f* v_head = &poly_in.verts[i];
    const Vec3f* v_tail = &poly_in.verts[(i + 1) % poly_in.num_verts];

    f32 d_head = calcSignedDistance(light_ortho_min, light_ortho_max, *v_head, plane);
    f32 d_tail = calcSignedDistance(light_ortho_min, light_ortho_max, *v_tail, plane);

    bool inside_head = d_head >= 0.0f;
    bool inside_tail = d_tail >= 0.0f;

    // 当前顶点若在裁剪面内则直接加入多边形
    if(inside_head) poly_out.verts[poly_out.num_verts++] = *v_head;

    // 若当前、下一个顶点不在同侧，说明此边与裁剪面有交点
    if(inside_head != inside_tail)
      poly_out.verts[poly_out.num_verts++] = Math::lerp(*v_head, *v_tail, d_head / (d_head - d_tail));
  }

  return poly_out;
}

constexpr std::array kClipPlanes {
  Left, Right, Top, Bottom
};

inline bool clip(const Vec2f& light_ortho_min,
                 const Vec2f& light_ortho_max,
                 ClipPolygon& tri) noexcept {
  int code0 = calcOutcode(light_ortho_min, light_ortho_max, tri.verts[0]);
  int code1 = calcOutcode(light_ortho_min, light_ortho_max, tri.verts[1]);
  int code2 = calcOutcode(light_ortho_min, light_ortho_max, tri.verts[2]);

  // 三个顶点均在某裁剪面外侧，不可见直接丢弃
  if((code0 & code1 & code2) != None) return false;

  int union_code = code0 | code1 | code2;
  // 三个顶点都不在任意一个裁剪面外部，则面片在视椎内，无需裁剪
  if(union_code == None) return true;

  for(const auto& plane: kClipPlanes) {
    if((union_code & plane) == None) continue;
    tri = clipByPlane(light_ortho_min, light_ortho_max, tri, plane);
    if(tri.num_verts < 3) return false;
  }
  return true;
}

}
