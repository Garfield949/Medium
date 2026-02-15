///
/// @file Mesh.h
/// @brief 网格类型
///
#pragma once

#include <filesystem>

#include "Core/Primitivies/Vertex.h"

namespace Medium {

    /// @class Mesh
    /// @brief 网格资源类型
    /// @details 存储网格顶点和索引，图元为三角形
    class Mesh {
    public:
        // 返回三角形总数
        [[nodiscard]] size_t triangleCount() const noexcept {
            return indices.size() / 3;
        }

        // 静态工厂，加载 Wavefront OBJ 模型文件
        static std::shared_ptr<Mesh> loadFromObj(const std::filesystem::path& path);

        std::vector<Vertex> vertices; // 去重后的顶点数据
        std::vector<size_t> indices;  // 索引，存的是对应顶点在 vertices 里的下标

    private:
        Mesh() = default;
    };

} // namespace