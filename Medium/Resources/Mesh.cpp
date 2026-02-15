///
/// @file Mesh.cpp
///
#include "Mesh.h"

#include <iostream>
#include <unordered_map>

// 定义此宏以在该文件内展开 TinyObjLoader 的具体实现；
// 注意只能在一个文件内定义此宏（比如该文件内），以避免多重定义。
#define TINYOBJLOADER_IMPLEMENTATION
#include <TinyObjLoader.h>
#include <fmt/core.h>

namespace Medium {

    std::shared_ptr<Mesh> Mesh::loadFromObj(const std::filesystem::path& path) {
        using namespace tinyobj;

        struct IndexHash {  // 定义一个 Hash 仿函数，() 被重载以创建函数对象，注意重载函数必须为 const
            size_t operator()(const index_t& idx) const noexcept {
                // {} 为初始化列表，这里是实例化一个计算 int 的临时对象，() 调用了
                // 为 int 重载的 operator().
                const size_t h1 = std::hash<int>{}(idx.vertex_index);
                const size_t h2 = std::hash<int>{}(idx.normal_index);
                const size_t h3 = std::hash<int>{}(idx.texcoord_index);
                // boost::hash_combine 风格
                auto combine = [](size_t h, size_t v) {
                    return h ^ (v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
                };
                size_t h = combine(h1, h2);
                h = combine(h, h3);
                return h;
            }
        };
        struct IndexEqual { // 定义重载比较运算符的仿函数
            bool operator()(const index_t& lhs, const index_t& rhs) const noexcept {
                // 仅索引相同还不够，因为同一个空间位置可能被多个面共用，而其在每个面上的法线和 uv 都可能不同
                // 这样的是会被算作两个不同的顶点。
                return lhs.vertex_index == rhs.vertex_index &&
                       lhs.normal_index == rhs.normal_index &&
                       lhs.texcoord_index == rhs.texcoord_index;
            }
        };

        ObjReader reader;          // OBJ 读取器
        ObjReaderConfig config;    // OBJ 配置器
        config.mtl_search_path = "";
        config.triangulate = true; // 确保网格的基本图元是三角形

        // OBJ 读取错误处理
        if(!reader.ParseFromFile(path, config)) {
            if(!reader.Error().empty())
                fmt::print(stderr, "\nMesh::loadFromObj - TinyObjReader::{}\n", reader.Error());
            return nullptr;
        }

        // OBJ 警告处理
        if(!reader.Warning().empty())
            fmt::print("TinyObjReader::{}\n", reader.Warning());

        // attrib_t 存储实际的顶点数据（坐标、法线、纹理等）；
        // 注意：attrib_t 的顶点数组 std::vector<real_t> vertices 存放的是平铺
        // 的分量数据，即 x0, y0, z0, x1, y1, z1, ... 法线和纹理同理。
        auto& attrib = reader.GetAttrib();

        // 打印调试信息
        fmt::print("\nMesh::loadFromObj - [{}] - Success.\n"
                   "Vertices: {}\n"
                   "Normals : {}\n"
                   "UVs     : {}\n\n", path.string(),
                                       attrib.vertices.size()/3, attrib.normals.size()/3, attrib.texcoords.size()/2);

        // shape_t 有 Mesh、Lines、Points。但大多数模型都只以 Mesh 来表示；
        // shape_t 里的 Mesh 类型存放顶点索引；
        // 一个模型可能有多个 shape。比如头、身、四肢，每个 shape 有独立的 mesh，所以 shapes 的大小很可能是 1
        auto& shapes = reader.GetShapes();

        size_t total_indices = 0; // 预留空间尺寸计算。
        for(const auto& shape : shapes) total_indices += shape.mesh.indices.size();

        Mesh mesh; // 空间预分配。顶点数一般是索引数的 1/3，参考一个正方体。
        mesh.indices.reserve(total_indices);
        mesh.vertices.reserve(total_indices / 3);

        // 负责存储 index -> 对应顶点在 mesh.vertices 中下标的对应键值；映射表 unordered_map 不允许重复键值
        // 对，所以能确保绝对的一对一关系。
        std::unordered_map<index_t, size_t, IndexHash, IndexEqual> unique_map;

        // 遍历 shape
        for(const auto& shape: shapes)
            for(const auto& index : shape.mesh.indices) { // 每个 index_t 对应一个顶点的三种属性：position, normal, uv.
                if(unique_map.count(index) == 0) {        // count 只能返回 0 或 1.
                    // Lambda，根据索引类型组装值并返回
                    auto fetch = [&](const std::vector<real_t>& data, int idx_type, int stride) -> Vec3f {
                        // 当 idx_type = -1 即法线或纹理不存在时，返回零向量。
                        if(idx_type < 0)
                            return Vec3f::Zero();
                        Vec3f res;
                        for(int j = 0; j < stride; ++j) {
                            res[j] = data[stride * idx_type + j];
                        }
                        return res;
                    };
                    mesh.vertices // 原地构造一个 Vertex 对象。
                        .emplace_back(fetch(attrib.vertices, index.vertex_index, 3),
                                       fetch(attrib.normals, index.normal_index, 3),
                                     fetch(attrib.texcoords, index.texcoord_index, 2).head<2>());

                    size_t new_vert_idx = mesh.vertices.size() - 1; // 获得最新存入顶点的下标（即 vector 尾部）
                    unique_map[index] = new_vert_idx;               // unordered_map 存入键值对
                    mesh.indices.push_back(new_vert_idx);           // Mesh 的索引数组存入对应顶点在 mesh.vertices 中的下标
                } else
                    mesh.indices.push_back(unique_map[index]);      // 已存在则直接存入对应顶点在 mesh.vertices 中的下标
            }
        return std::make_shared<Mesh>(std::move(mesh));
    }

} // namespace Medium
