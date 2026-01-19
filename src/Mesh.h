#pragma once

#include "Vertex.h"

namespace Medium {

    /// @struct Mesh
    /// @brief 网格类型
    /// @details 存储网格顶点和索引，图元为三角形
    struct Mesh {
        // 返回三角形总数
        [[nodiscard]] constexpr size_t triangleCount() const noexcept {
            return vertices.size() / 3;
        }
        // 顶点容器
        std::vector<Vertex> vertices;
    };

} // namespace