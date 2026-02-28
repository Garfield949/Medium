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
        Vertex(Vec3f position, Vec3f normal, Vec2f tex_coord)
            : pos(std::move(position)), normal(std::move(normal)), tex_coord(std::move(tex_coord)) {}
        // 直接传分量
        Vertex(f32 vx, f32 vy, f32 vz,
               f32 nx = 0, f32 ny = 0, f32 nz = 0,
               f32 tu = 0, f32 tv = 0)
            : pos{vx, vy, vz}, normal{nx, ny, nz}, tex_coord{tu, tv} {}
        // 默认构造
        Vertex() = default;

        // 位置
        Vec3f pos{};
        // 法线
        Vec3f normal{};
        // 纹理坐标
        Vec2f tex_coord{};
        // 顶点默认白色
        Color color{255};
    };

} // namespace Medium
