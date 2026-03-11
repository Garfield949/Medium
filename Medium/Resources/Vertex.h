///
/// @file Vertex.h
/// @brief 顶点类型
///
#pragma once

#include <utility>

#include "Base/Types.h"
#include "Base/Color.h"

namespace Medium {

  /// @struct Vertex
  /// @brief 顶点类型
  /// @details 存储位置、法线、UV和颜色
  struct Vertex {
    Vertex() = default;
    Vertex(Vec3f position, Vec3f normal, Vec2f tex_coord)
      : pos(std::move(position)), normal(std::move(normal)), uv(std::move(tex_coord)) {} 
    
    Vec3f pos     = Vec3f::Zero(); // 位置
    Vec3f normal  = Vec3f::Zero(); // 法线
    Vec4f tangent = Vec4f::Zero(); // 切线，w 分量存储手性
    Vec2f uv      = Vec2f::Zero(); // 纹理坐标

    LinearColor color{1.0f};
  };

} // namespace Medium
