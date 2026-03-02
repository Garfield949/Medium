///
/// @file Mesh.cpp
///
#include "Mesh.h"

#include <cmath>
#include <unordered_map>

// 定义此宏以在该文件内展开 TinyObjLoader 的具体实现；
// 注意只能在一个文件内定义此宏（比如该文件内），以避免多重定义。
#define TINYOBJLOADER_IMPLEMENTATION
#include <TinyObjLoader.h>
#include <fmt/core.h>
#include <fmt/color.h>

#include "Resources/Material.h"
#include "Resources/Texture.h"

namespace Medium {

    MeshPtr Mesh::loadFromObj(const std::filesystem::path& obj_path) {
        using namespace tinyobj;
        ObjReader reader;                                         // OBJ 读取器
        ObjReaderConfig config;                                   // OBJ 配置器
        config.mtl_search_path = obj_path.parent_path().string(); // .mtl 与 .obj 于同一文件夹下
        config.triangulate = true;                                // 确保网格的基本图元是三角形

        if(!reader.ParseFromFile(obj_path, config)) {
            if(!reader.Error().empty())
                fmt::print(stderr, "\nMesh::loadFromObj - TinyObjReader::{}\n", reader.Error());
            return nullptr;
        }
        // if(!reader.Warning().empty())
        //     fmt::print("Mesh::loadFromObj - TinyObjReader::{}\n", reader.Warning());

        // 1. attrib_t 存储实际的顶点数据（坐标、法线、纹理等）。注意：attrib_t 的顶点数组 std::vector<real_t> vertices
        //    存放的是平铺的分量数据，即 x0, y0, z0, x1, y1, z1, ... 法线和纹理同理；
        // 2. shape_t 有 Mesh、Lines、Points。但大多数模型都只以 Mesh 来表示，其中 Mesh 类型存放顶点索引；
        // 3. 一个模型可能有多个 shape。比如头、身、四肢，每个 shape 有独立的 mesh，所以 shapes 的大小有可能是 1; 在 .obj
        //    文件中，每一个 o 或 g 开头的 f 面片分组会被识别为一个 shape.
        // 4. materials 由识别 .mtl 得到。
        auto& attrib = reader.GetAttrib();
        auto& shapes = reader.GetShapes();
        auto& materials = reader.GetMaterials();

        // 打印调试信息
        fmt::print("\nMesh::loadFromObj - [{}] - Success.\n"
                   "Vertices : {}\n"
                   "Normals  : {}\n"
                   "UVs      : {}\n"
                   "Submeshes: {}\n"
                   "Materials: {}\n\n", obj_path.string(),       attrib.vertices.size()/3,
                                        attrib.normals.size()/3, attrib.texcoords.size()/2,
                                        shapes.size(),           materials.size());

        // 顶点、索引数组空间预计算。
        size_t total_indices = 0;
        for(const auto& shape : shapes)
            total_indices += shape.mesh.indices.size();

        Mesh mesh;
        mesh.indices.reserve(total_indices);
        mesh.vertices.reserve(total_indices / 3); // 顶点数一般是索引数的 1/3，参考一个正方体。

        // 读入材质 (MTL)
        for(const auto & mat_t : materials) {
            auto m = std::make_shared<Material>();

            m->name = mat_t.name;

            // 将 TinyObjLoader 读取的原始材质参数装入 MaterialParams 结构。
            MaterialParams& mat_params = m->params;
            mat_params.Ka = {mat_t.ambient[0],  mat_t.ambient[1],  mat_t.ambient[2]};
            mat_params.Kd = {std::powf(mat_t.diffuse[0], 2.4), std::powf(mat_t.diffuse[1], 2.4), std::powf(mat_t.diffuse[2], 2.4)};
            mat_params.Ks = {mat_t.specular[0], mat_t.specular[1], mat_t.specular[2]};
            mat_params.Ke = {mat_t.emission[0], mat_t.emission[1], mat_t.emission[2]};

            mat_params.Ns = mat_t.shininess;
            mat_params.Ni = mat_t.ior;
            mat_params.d  = mat_t.dissolve;
            mat_params.Tf = mat_t.transmittance[0];
            mat_params.illum = mat_t.illum;

            // Lambda, 拼接材质完整路径。
            auto loadTex = [&](std::string path) -> TexturePtr {
                if(path.empty()) return nullptr;
                // 将反斜杠 '\' 替换为 '/' (Windows同样支持'/')
                std::replace(path.begin(), path.end(), '\\', '/');
                auto full_path = obj_path.parent_path() / path;
                return Texture::loadFromFile(full_path);
            };

            // 材质贴图指针先赋给 Material 对象，在 draw call 阶段再装配给 Uniform.
            m->map_Ka = loadTex(mat_t.ambient_texname);
            m->map_Kd = loadTex(mat_t.diffuse_texname);
            m->map_Ks = loadTex(mat_t.specular_texname);
            m->map_d = loadTex(mat_t.alpha_texname);
            m->map_bump = loadTex(mat_t.bump_texname);

            mesh.materials.push_back(std::move(m));
        }
        
        auto texStatus = [](const TexturePtr& tex) -> std::string {
            return tex ? " [X] " : " [ ] ";
        };

        fmt::print(fmt::emphasis::bold | fg(fmt::color::cyan), 
                   "--- Material Summary ({}) ---\n", mesh.materials.size());
        fmt::print("{:<20} | {:<5} | {:<5} | {:<5} | {:<5} | {:<5} | {:<5}\n", 
                   "Name", "Illum", "Diff", "Spec", "Norm", "Alpha", "Ambi");
        fmt::print("{:-<85}\n", "");
        
        for (const auto& m : mesh.materials) {
            fmt::print("{:<20} | {:<5} | {} | {} | {} | {} | {}\n",
                       m->name.substr(0, 20),
                       m->params.illum,
                       texStatus(m->map_Kd),
                       texStatus(m->map_Ks),
                       texStatus(m->map_bump),
                       texStatus(m->map_d),
                       texStatus(m->map_Ka));
        }
        fmt::print("\n");

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
        struct IndexEqual {  // 定义重载比较运算符的仿函数
            bool operator()(const index_t& lhs, const index_t& rhs) const noexcept {
                // 仅索引相同还不够，因为同一个空间位置可能被多个面共用，而其在每个面上的法线和 uv 都可能不同
                // 这样的是会被算作两个不同的顶点。
                return lhs.vertex_index == rhs.vertex_index &&
                       lhs.normal_index == rhs.normal_index &&
                       lhs.texcoord_index == rhs.texcoord_index;
            }
        };

        // 负责存储 index -> 对应顶点在 mesh.vertices 中下标的对应键值。
        std::unordered_map<index_t, size_t, IndexHash, IndexEqual> unique_map;

        // 遍历 shapes. o 或 g 开头的 f 面片分组会被识别为一个 shape.
        for(const auto& shape: shapes) {

            // 将材质相同的面分装在一起 [material_id] -> [vector]. vector 中每 3 个 index_t 为一个 face.
            std::unordered_map<int, std::vector<index_t>> faces_by_material;

            // 遍历 shape 里的每个面。num_face_vertices 是一个 vector，num_face_vertices[i] 表示第 i 个
            // 面有几个顶点。由于已配置了三角化，这里只用到 num_face_vertices.size()，即获取总三角面片的个数。
            for(auto f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
                // 由面片的序号拿到对应的材质id. 如果面片没有材质，tinyobjloader 会将其 mat_id 置 -1.
                auto mat_id = shape.mesh.material_ids[f];

                // 防御性编程。如果是标准模型，mat_id 要么是 -1，要么是在 0 ~ materials.size()-1 范围内。
                if(mat_id < 0 || mat_id >= static_cast<int>(materials.size()))
                    mat_id = -1;

                // 属于同一材质的三角面装到一个桶中。
                for(auto v = 0; v < 3; v++)
                    faces_by_material[mat_id].push_back(shape.mesh.indices[f * 3 + v]);
            }

            // 将材质相同的面组装为一个 SubMesh.
            for(const auto & [mat_id, face_indices] : faces_by_material) {
                SubMesh sub_mesh;
                sub_mesh.index_offset   = mesh.indices.size();
                sub_mesh.index_count    = face_indices.size();
                sub_mesh.material_index = mat_id;

                // 将每个 SubMesh 顶点去重并计算顶点索引。
                for(const auto& index : face_indices) {
                    if(unique_map.count(index) == 0) {  // count 只能返回 0 或 1.

                        // Lambda，根据索引类型组装值并返回
                        auto fetch = [&](const std::vector<real_t>& data, int idx_type, int stride) -> Vec3f {
                            // 当 idx_type = -1 即法线或纹理不存在时，返回零向量。
                            if(idx_type < 0)
                                return Vec3f::Zero();
                            Vec3f result;
                            for(int j = 0; j < stride; ++j) {
                                result[j] = data[stride * idx_type + j];
                            }
                            return result;
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
                mesh.sub_meshes.push_back(sub_mesh);
            }
        }

        // 若模型不自带法线就手动计算。
        if(attrib.normals.empty()) {
            fmt::print("Mesh::loadFromObj - No normals detected in [{}]\n"
                "Vertex normals will be generated automatically.\n\n", obj_path.string());

            mesh.calculateNormals();
        }

        return std::make_shared<Mesh>(std::move(mesh));
    }

    // 面积加权平均法线 Area-weighted vertex normals. 对于每个三角面片，计算其法向，并累加到三个顶点的法向上。
    // 最后将所有顶点的法向归一化。
    void Mesh::calculateNormals() {
        for(size_t f = 0; f < indices.size(); f += 3) {
            const Vec3f& pos0 = vertices[indices[f + 0]].pos;
            const Vec3f& pos1 = vertices[indices[f + 1]].pos;
            const Vec3f& pos2 = vertices[indices[f + 2]].pos;

            Vec3f face_normal = (pos1 - pos0).cross(pos2 - pos0);

            for(size_t j = 0; j < 3; ++j)
                vertices[indices[f + j]].normal += face_normal;
        }
        for(auto & vertex : vertices) vertex.normal.normalize();
    }

} // namespace Medium