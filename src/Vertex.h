#pragma once

#include <utility>

#include "Types.h"
#include "Color.h"

namespace Medium {

    /// @struct Vertex
    /// @brief 顶点类型
    /// @details 存储位置、法线、UV和颜色
    struct Vertex {
        // 参数均为 POD, Plain Old Data 类型，移动和拷贝的开销没有差别，目前
        // 没必要使用 std::move
        Vertex(Vec3f p, Vec3f n, Vec2f  t)
            : pos(std::move(p)), normal(std::move(n)), tex_coord(std::move(t)) {}
        // 直接传分量
        Vertex(f32 vx, f32 vy, f32 vz,
               f32 nx = 0, f32 ny = 0, f32 nz = 0,
               f32 tu = 0, f32 tv = 0)
            : pos{vx, vy, vz}, normal{nx, ny, nz}, tex_coord{tu, tv} {}
        // 默认构造
        Vertex() = default;

        // 位置
        Vec3f pos;
        // 法线
        Vec3f normal;
        // 纹理坐标
        Vec2f tex_coord;
        // 顶点默认白色
        Color color{255};
    };

} // namespace Medium
