///
/// @file   ShaderTypes.h
/// @author Garfield949
/// @date   2026/3/2 13:17
/// @brief
///
#pragma once

#include "Base/Types.h"

namespace Medium {

struct Varying {
  Vec2f uv        = Vec2f::Zero(); // 纹理坐标
  Vec3f normal    = Vec3f::Zero(); // 顶点变到世界空间里的法向
  Vec4f tangent   = Vec4f::Zero(); //
  Vec3f pos_world = Vec3f::Zero(); // 某些计算（比如光照）依赖顶点世界坐标
  f32   w_inv     = 0.0f;          // 保存顶点的 1/w
};

struct VSOut {
  Vec4f   pos_clip = Vec4f::Zero();
  Varying varying  = {};
};

} // namespace Medium
