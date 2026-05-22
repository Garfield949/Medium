///
/// @file   Clip.h
/// @author Garfield949
/// @date   2026/3/3 21:27
/// @brief  
///
#pragma once

#include "Base/Math/Math.h"
#include "Pipeline/Shader/ShaderTypes.h"

namespace Medium {

// 定义裁剪空间的视椎平面
enum class ClipPlane: u8 {
  Left   = 1 << 0, 
  Right  = 1 << 1, 
  Top    = 1 << 2, 
  Bottom = 1 << 3, 
  Near   = 1 << 4, 
  Far    = 1 << 5,
  None   = 0
};
inline ClipPlane operator|(ClipPlane a, ClipPlane b) noexcept {
  return static_cast<ClipPlane>(static_cast<u8>(a) | static_cast<u8>(b));
}
inline ClipPlane& operator|=(ClipPlane& a, ClipPlane b) noexcept {
  a = a | b; return a;
}
inline ClipPlane operator&(ClipPlane a, ClipPlane b) noexcept {
  return static_cast<ClipPlane>(static_cast<u8>(a) & static_cast<u8>(b));
}

/// @struct ClipVertex
/// @brief 裁剪过程中，三角形的某边若越过了裁剪面，则需在相交处生成新顶点。该顶点的 Clip Space 坐标
///        及 varying 由线性插值得到。
struct ClipVertex {
  // 1/w 在 clip space 里是非线性的，不能直接插值。这里先对 pos 进行插值，再取 1/w
  // Note: 不对插值过程中的 normal 和 tangent 进行归一化，因为 fragment 阶段会归一化
  // TODO: 让 Varying 自己负责插值
  [[nodiscard]] static ClipVertex lerp(const ClipVertex& a, const ClipVertex& b, f32 t) noexcept {
    Vec4f pos_lerp = Math::lerp(a.pos, b.pos, t);
    const Varying& var_a = a.varying;
    const Varying& var_b = b.varying;
    return {
      pos_lerp,
      Varying{
        Math::lerp(var_a.uv,        var_b.uv,        t),
        Math::lerp(var_a.normal,    var_b.normal,    t),
        Math::lerp(var_a.tangent,   var_b.tangent,   t),
        Math::lerp(var_a.pos_world, var_b.pos_world, t),
        1.0f / pos_lerp.w()
      }
    };
  }
  Vec4f pos;
  Varying varying;
};

/// @struct ClipPolygon
/// @brief 三角形初始有 3 个顶点，每次与裁剪面相交最多新产生 1 个顶点；遍历 6 个面便会最多得到 9 个顶点
inline constexpr size_t MaxClipVertsNum = 9;
struct ClipPolygon {
  std::array<ClipVertex, MaxClipVertsNum> verts{};
  size_t num_verts = 0;
};

inline ClipPlane calcOutcode(const Vec4f& v) noexcept {
  ClipPlane code = ClipPlane::None;
  if(v.x() < -v.w()) code |= ClipPlane::Left;
  if(v.x() >  v.w()) code |= ClipPlane::Right;
  if(v.y() >  v.w()) code |= ClipPlane::Top;
  if(v.y() < -v.w()) code |= ClipPlane::Bottom;
  if(v.z() >  v.w()) code |= ClipPlane::Near;
  if(v.z() <  0.0f)  code |= ClipPlane::Far;
  return code;
}

inline constexpr std::array<ClipPlane, 6> ClipPlanes = {
  ClipPlane::Near,   ClipPlane::Left, 
  ClipPlane::Right,  ClipPlane::Top, 
  ClipPlane::Bottom, ClipPlane::Far,
};

[[nodiscard]] inline f32 calcSignedDistance(const Vec4f& v, ClipPlane plane) noexcept {
  switch(plane) {
    case ClipPlane::Left:   return v.w() + v.x();
    case ClipPlane::Right:  return v.w() - v.x();
    case ClipPlane::Top:    return v.w() - v.y();
    case ClipPlane::Bottom: return v.w() + v.y();
    case ClipPlane::Near:   return v.w() - v.z();
    case ClipPlane::Far:    return v.z();
    default: return 0.0f;
  }
}

[[nodiscard]] inline ClipPolygon clipByPlane(const ClipPolygon& poly_in, ClipPlane plane) noexcept {
  ClipPolygon poly_out;
  
  // 遍历输入多边形的所有边，由 head -> tail 定义
  for(size_t i = 0; i < poly_in.num_verts; ++i) {
    // 当 i = num_verts - 1, (i + 1) % num_verts = 0，尾首完成闭环
    const ClipVertex* v_head = &poly_in.verts[i];
    const ClipVertex* v_tail = &poly_in.verts[(i + 1) % poly_in.num_verts];
    
    f32 d_head = calcSignedDistance(v_head->pos, plane);
    f32 d_tail = calcSignedDistance(v_tail->pos, plane);
    
    bool inside_head = d_head >= 0.0f;
    bool inside_tail = d_tail >= 0.0f;
    
    // 当前顶点若在裁剪面内则直接加入多边形
    if(inside_head) poly_out.verts[poly_out.num_verts++] = *v_head;
    
    // 若当前、下一个顶点不在同侧，说明此边与裁剪面有交点
    if(inside_head != inside_tail)
      poly_out.verts[poly_out.num_verts++] = ClipVertex::lerp(*v_head, *v_tail, d_head / (d_head - d_tail));
  }
  
  return poly_out;
}

// 输入初始三角面片
[[nodiscard]] inline bool clip(ClipPolygon& tri) noexcept {
  using Outcode = ClipPlane;
  
  Outcode code0 = calcOutcode(tri.verts[0].pos);
  Outcode code1 = calcOutcode(tri.verts[1].pos);
  Outcode code2 = calcOutcode(tri.verts[2].pos);
  
  // 三个顶点均在某裁剪面外侧，不可见直接丢弃
  if((code0 & code1 & code2) != ClipPlane::None) return false;
  
  Outcode union_code = code0 | code1 | code2;
  // 三个顶点都不在任意一个裁剪面外部，则面片在视椎内，无需裁剪
  if(union_code == ClipPlane::None) return true;
  
  // 剩下的情况：至少有一条边跨越了某个裁剪面。
  // 依次被六个面裁剪，若某次被裁到只剩不到三个顶点，说明三角形不在视椎体内部:
  // 1. 只有一个顶点：多边形只有一个顶点落在裁剪面上，其它顶点都在视椎外部；
  // 2. 只有两个顶点：多边形有一条边落在裁剪面上，其它顶点在视椎外部。
  for(const auto& plane: ClipPlanes) {
    if((union_code & plane) == ClipPlane::None) continue;
    tri = clipByPlane(tri, plane);
    if(tri.num_verts < 3) return false;
  }
  return true;
}

} // namespace Medium
