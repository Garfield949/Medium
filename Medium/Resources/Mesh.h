///
/// @file Mesh.h
/// @brief 网格类型
///
#pragma once

#include <filesystem>

#include "Core/Primitivies/Vertex.h"
#include "Core/Forwards.h"

namespace Medium {

    struct SubMesh {
        [[nodiscard]] size_t triangleCount() const noexcept {
            return index_count / 3;
        }

        u32 index_offset    = 0;
        u32 index_count     = 0;
        int material_index  = 0;
    };

    /// @class Mesh
    /// @brief 网格资源类型
    /// @details 存储网格顶点和索引，图元为三角形
    class Mesh {
    public:

        // 静态工厂，加载 Wavefront OBJ 模型文件
        static MeshPtr loadFromObj(const std::filesystem::path& obj_path);

        std::vector<SubMesh>     sub_meshes; // 子网格数据
        std::vector<MaterialPtr> materials;  // 子网格材质

        std::vector<Vertex> vertices; // 去重后的顶点数据
        std::vector<size_t> indices;  // 索引，存的是对应顶点在 vertices 里的下标

    private:
        Mesh() = default;
        void calculateNormals();
    };

} // namespace