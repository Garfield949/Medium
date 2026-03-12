///
/// @file   Clip.h
/// @author Garfield949
/// @date   2026/3/3 21:27
/// @brief  
///
#pragma once

#include "Pipeline/Shader/BlinnPhong.h"

namespace Medium {
  
  /// @struct ClipVertex
  /// @brief 裁剪过程中，三角形的某边若越过了裁剪面，则需在相交处生成新顶点。该顶点的 Clip Space 坐标
  ///        及 varying 由线性插值得到。
  struct ClipVertex {
    ClipVertex() = default;
    
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
    
    Vec4f   pos;
    Varying varying;
  };
  
  /// @struct ClipPolygon
  /// @brief 三角形初始有 3 个顶点，每次与裁剪面相交最多新产生 1 个顶点；遍历 6 个面便会最多得到 9 个顶点
  inline constexpr size_t MaxClipVertsNum = 9;
  struct ClipPolygon {
    std::array<ClipVertex, MaxClipVertsNum> verts{};
    u16 num_verts = 0;
  };
  
  enum class ClipPlane {
    Left, Right, Top, Bottom, Near, Far,
  };
  
  inline constexpr std::array<ClipPlane, 6> ClipPlanes = {
    ClipPlane::Near,   ClipPlane::Left, 
    ClipPlane::Right,  ClipPlane::Top, 
    ClipPlane::Bottom, ClipPlane::Far,
  };
  
  inline f32 calcSignedDistance(const Vec4f& v, ClipPlane plane) {
    switch(plane) {
      case ClipPlane::Left:   return v.x() + v.w();
      case ClipPlane::Right:  return v.w() - v.x();
      case ClipPlane::Top:    return v.w() - v.y();
      case ClipPlane::Bottom: return v.y() + v.w();
      case ClipPlane::Near:   return v.w() - v.z();
      case ClipPlane::Far:    return v.z();
      default: return 0.0f;
    }
  }
  
  [[nodiscard]] inline ClipPolygon clipByPlane(const ClipPolygon& poly_in, ClipPlane plane) {
    ClipPolygon poly_out;
    
    // 遍历输入多边形的所有边，由 head -> tail 定义
    for(int i = 0; i < poly_in.num_verts; ++i) {
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
  [[nodiscard]] inline bool clip(ClipPolygon& tri) {
    // 依次被六个面裁剪，若某次被裁到只剩不到三个顶点，说明三角形不在视椎体内部:
    // 1. 只有一个顶点：多边形只有一个顶点落在裁剪面上，其它顶点都在视椎外部；
    // 2. 只有两个顶点：多边形有一条边落在裁剪面上，其它顶点在视椎外部。
    for(const auto & plane : ClipPlanes) {
      tri = clipByPlane(tri, plane);
      if(tri.num_verts < 3) return false;
    }
    return true;
  }

} // namespace Medium
