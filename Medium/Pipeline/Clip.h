///
/// @file   Clip.h
/// @author Garfield949
/// @date   2026/3/3 21:27
/// @brief  
///
#pragma once

#include "Pipeline/Shader/ShaderTypes.h"

namespace Medium {

/// @enum ClipPlane
/// @brief   定义裁剪空间的视椎平面。
/// @details 注：使用 enum 而非 enum class 时，位运算结果默认是一个 int.
enum ClipPlane {
  Left   = 1 << 0, 
  Right  = 1 << 1, 
  Top    = 1 << 2, 
  Bottom = 1 << 3, 
  Near   = 1 << 4, 
  Far    = 1 << 5,
  None   = 0
};

/// @struct ClipVertex
/// @brief  裁剪过程中，三角形的某边若越过了裁剪面，则需在相交处生成新顶点。该顶点的 Clip Space 坐标及 varying 由线性插值得到。
struct ClipVertex {
  static ClipVertex Lerp(const ClipVertex& a, const ClipVertex& b, f32 t, i32 varying_scalar_count) noexcept;
  Vec4f pos;
  Varying varying;
};

/// @struct ClipPolygon
/// @details 三角形初始有 3 个顶点，每次与裁剪面相交最多新产生 1 个顶点；遍历 6 个面便会最多得到 9 个顶点
inline constexpr u32 kMaxClipVertsNum = 9;
struct ClipPolygon {
  std::array<ClipVertex, kMaxClipVertsNum> verts;
  i32 num_verts = 0;
};

/// @brief 三角面片裁剪入口。
/// @details return false 代表三角形被完全裁掉了。
bool clipTriangle(ClipPolygon& tri, i32 varying_scalar_count) noexcept;

/// @brief Liang–Barsky / Cyrus–Beck 线段裁剪入口。算法非常抽象，得从实际例子理解。
/// @details return false 代表线段被完全裁掉了。
bool clipLine(Vec4f& v0, Vec4f& v1) noexcept;

} // namespace Medium
