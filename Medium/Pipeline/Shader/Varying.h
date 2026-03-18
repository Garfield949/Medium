///
/// @file   Varying.h
/// @author Garfield949
/// @date   2026/3/17 16:41
/// @brief  
///
#pragma once

#include "Base/Types.h"

namespace Medium {

  struct Varying {
    Vec2f uv;        // 纹理坐标
    Vec3f normal;    // 顶点变到世界空间里的法向
    Vec4f tangent;   //
    Vec3f pos_world; // 某些计算（比如光照）依赖顶点世界坐标
    f32   w_inv;     // 保存顶点的 1/w
  };

} // namespace Medium
