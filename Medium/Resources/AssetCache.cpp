///
/// @file   AssetCache.cpp
/// @author Garfield949
/// @date   2026/3/3 23:36
/// @brief  
///
#include "AssetCache.h"

#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#include <TinyObjLoader.h>
#include <stb_image.h>
#include <fmt/color.h>

#include "Base/Color.h"
#include "TextureSlot.h"
#include "Mesh.h"
#include "Material.h"

namespace Medium {

  MeshPtr AssetCache::loadObj(const std::filesystem::path& obj_path) {
    
    if(auto it = mesh_cache_.find(obj_path.string()); it != mesh_cache_.end()) {
      // 检查资源是否存在，是就提升为 shared_ptr，否则 lock 返回 nullptr
      if(auto mesh_cached = it->second.lock())
        return mesh_cached;
      // Key 存在，但对应 value 已被析构，则删掉键值对
      mesh_cache_.erase(it);
    }

    MeshPtr mesh_new{new Mesh};
    mesh_new->name = obj_path.filename();
    mesh_cache_[obj_path.string()] = mesh_new;

    using namespace tinyobj;
    ObjReader reader;                                         // OBJ 读取器
    ObjReaderConfig config;                                   // OBJ 配置器
    config.mtl_search_path = obj_path.parent_path().string(); // .mtl 与 .obj 于同一文件夹下
    config.triangulate = true;                                // 确保网格的基本图元是三角形

    if(!reader.ParseFromFile(obj_path, config)) {
      if(!reader.Error().empty())
        fmt::print(stderr, "\nAssetCache::loadFromObj - TinyObjReader::{}\n", reader.Error());
      return nullptr;
    }

    // 1. attrib_t 存储实际的顶点数据（坐标、法线、纹理等）。注意：attrib_t 的顶点数组 std::vector<real_t> vertices
    //    存放的是平铺的分量数据，即 x0, y0, z0, x1, y1, z1, ... 法线和纹理同理；
    // 2. shape_t 有 Mesh、Lines、Points。但大多数模型都只以 Mesh 来表示，其中 Mesh 类型存放顶点索引；
    // 3. 一个模型可能有多个 shape。比如头、身、四肢，每个 shape 有独立的 mesh，所以 shapes 的大小有可能是 1; 在 .obj
    //    文件中，每一个 o 或 g 开头的 f 面片分组会被识别为一个 shape.
    // 4. materials 由识别 .mtl 得到。
    auto& attrib    = reader.GetAttrib();
    auto& shapes    = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    // 打印 obj 信息
    fmt::print(fmt::emphasis::bold | fg(fmt::color::cyan), "\nAssetCache::loadFromObj - [{}]\n", obj_path.string());
    fmt::print("Vertices : {}\n"
               "Normals  : {}\n"
               "UVs      : {}\n"
               "Submeshes: {}\n"
               "Materials: {}\n", attrib.vertices.size()/3, attrib.normals.size()/3, attrib.texcoords.size()/2,
                                    shapes.size(), materials.size());

    // 顶点、索引数组空间预计算。
    size_t total_indices = 0;
    for(const auto& shape : shapes)
      total_indices += shape.mesh.indices.size();
    mesh_new->indices.reserve(total_indices);
    mesh_new->vertices.reserve(total_indices / 3); // 顶点数一般是索引数的 1/3，参考一个正方体。

    // 解析 MTL
    parseMTL(mesh_new, materials, obj_path);

    // 定义一个 Hash 仿函数，() 被重载以创建函数对象，注意重载函数必须为 const
    struct IndexHash {  
      size_t operator()(const index_t& idx) const noexcept {
        // {} 为初始化列表，这里是实例化一个计算 int 的临时对象，() 调用了为 int 重载的 operator().
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
        // 仅索引相同还不够，因为同一个空间位置可能被多个面共用，而其在每个面上的法线和 uv 都可能不同，这样会被算作两个不同的顶点。
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

        // 如果是标准模型，mat_id 要么是 -1，要么是在 0 ~ materials.size()-1 范围内
        // 无材质的面片一律撞到默认材质的桶中
        if(mat_id < 0 || mat_id >= static_cast<int>(materials.size())) {
          for(auto v = 0; v < 3; v++)
            faces_by_material[0].push_back(shape.mesh.indices[f * 3 + v]);
          continue;
        }
        // 属于同一材质的三角面装到一个桶中。
        for(auto v = 0; v < 3; v++)
          faces_by_material[mat_id + 1].push_back(shape.mesh.indices[f * 3 + v]);
      }

      // 将材质相同的面组装为一个 SubMesh.
      for(const auto & [mat_id, face_indices] : faces_by_material) {
        SubMesh sub_mesh;
        sub_mesh.index_offset   = mesh_new->indices.size();
        sub_mesh.index_count    = face_indices.size();
        sub_mesh.material_index = mat_id;

        // 将每个 SubMesh 顶点去重并计算顶点索引。
        for(const auto& index : face_indices) {
          if(unique_map.count(index) == 0) {  // count 只能返回 0 或 1.

            // Lambda，根据索引类型组装值并返回
            auto fetchValue = [&](const std::vector<real_t>& data, int idx_type, int stride) -> Vec3f {
              // 当 idx_type = -1 即法线或纹理不存在时，返回零向量。
              if(idx_type < 0)
                return Vec3f::Zero();
              Vec3f result;
              for(int j = 0; j < stride; ++j) {
                result[j] = data[stride * idx_type + j];
              }
              return result;
            };

            // 原地构造一个 Vertex 对象。
            mesh_new->vertices.emplace_back(fetchValue(attrib.vertices, index.vertex_index, 3),
                                            fetchValue(attrib.normals, index.normal_index, 3),
                                            fetchValue(attrib.texcoords, index.texcoord_index, 2).head<2>());

            size_t new_vert_idx = mesh_new->vertices.size() - 1; // 获得最新存入顶点的下标（即 vector 尾部）
            unique_map[index] = new_vert_idx;                    // unordered_map 存入键值对
            mesh_new->indices.push_back(new_vert_idx);           // Mesh 的索引数组存入对应顶点在 mesh.vertices 中的下标
          } else
            mesh_new->indices.push_back(unique_map[index]);      // 已存在则直接存入对应顶点在 mesh.vertices 中的下标
        }
        mesh_new->sub_meshes.push_back(sub_mesh);
      }
    }
    // 若模型不自带法线就手动计算。
    if(attrib.normals.empty()) {
      fmt::print("AssetCache::loadFromObj - No normals detected in [{}], "
        "Vertex normals will be generated automatically.\n\n", obj_path.string());
      mesh_new->calcNormals();
    }

    for(const auto& mat : mesh_new->materials)
      if(mat->enable_norm) {
        mesh_new->calcTangents();
        break;
      }

    return mesh_new;
  }

  void AssetCache::parseMTL(const MeshPtr&                          mesh_new,
                            const std::vector<tinyobj::material_t>& tiny_materials,
                            const std::filesystem::path&            obj_path)
  {
    mesh_new->materials.push_back(Material::Default());
    
    for(const auto & mat_t : tiny_materials) {
      if(auto it = material_cache_.find(mat_t.name); it != material_cache_.end()) {
        if(auto mat_cached = it->second.lock()) {
          // mat_cached 是由 lock 新构造的，所以完全可以 move
          mesh_new->materials.push_back(std::move(mat_cached));
          continue;
        }
        material_cache_.erase(it);
      }

      MaterialPtr mat_new = std::make_shared<Material>(mat_t.name);

      // 将 TinyObjLoader 读取的原始材质参数装入自定义结构
      MaterialParams& mat_new_params = mat_new->params;
      mat_new_params.Kd    = {mat_t.diffuse[0],  mat_t.diffuse[1],  mat_t.diffuse[2], 1.0f, ColorSpace::SRGB};
      mat_new_params.Ks    = {mat_t.specular[0], mat_t.specular[1], mat_t.specular[2]};
      mat_new_params.Ns    = mat_t.shininess;
      mat_new_params.illum = mat_t.illum;

      // Lambda: 拼接材质完整路径
      auto getTex = [&](std::string path_relative, ColorSpace space) -> TexturePtr {
        // 材质的某些通道（如漫反射）可能是不含材质图片的纯色，直接返回对应的空贴图指针即可；用 loadTexture 返回 nullptr 也行，但
        // 其会输出非法路径的错误信息，与语义不合。
        if(path_relative.empty()) return nullptr;
        // 在 Unix-like 系统中，'\' 被视为文件名的一部分，而非路径分隔符。
        std::replace(path_relative.begin(), path_relative.end(), '\\', '/');
        
        std::filesystem::path full_path;
        
        // 如果材质图的相对路径开头有 / 或 \ ，std::filesystem::path 会认为它是一个绝对路径，直接
        // 将 obj_path.parent_path() 丢掉。
        if(std::string str = path_relative; str[0] == '/' || str[0] == '\\') {
          str.erase(0, 1);
          full_path = obj_path.parent_path() / str;
        } else {
          // lexically_normal() 将类似 A//../b 的路径转为规范绝对路径
          full_path = (obj_path.parent_path() / path_relative).lexically_normal();
        }
        return loadTexture(full_path, space);
      };
      
      mat_new->diff_slot = makeSlot(getTex(mat_t.diffuse_texname,  ColorSpace::SRGB),   mat_new_params.Kd);
      mat_new->spec_slot = makeSlot(getTex(mat_t.specular_texname, ColorSpace::Linear), mat_new_params.Ks);
      mat_new->diss_slot = makeSlot(getTex(mat_t.alpha_texname,    ColorSpace::Linear), LinearColor{mat_t.dissolve});
      mat_new->norm_slot = getTex(mat_t.normal_texname, ColorSpace::Linear);
      mat_new->bump_slot = getTex(mat_t.bump_texname,   ColorSpace::Linear);

      mat_new->setEnables();

      mesh_new->materials.push_back(std::move(mat_new));

      // 不同模型的 MTL 里完全可能会有相同的材质名，因此用 filepath + name 作为 key
      material_cache_[obj_path.string() + mat_t.name] = mat_new;
    }
  }
  
  TexturePtr AssetCache::loadTexture(const std::filesystem::path& path, ColorSpace& space) {
    if(auto it = texture_cache_.find(path.string()); it != texture_cache_.end()) {
      if(auto tex_cached = it->second.lock())
        return tex_cached;
      texture_cache_.erase(it);
    }

    // Texture 有一个 std::unique_ptr 成员，默认是不允许拷贝的，会默认禁用掉移动构造，无法用 make_shared.
    TexturePtr tex_new{new Texture};

    // UV 空间的原点在左下角，而纹理图的存储是行优先的，原点在左上角。为了索引方便，先设置将读进的纹理上下翻转。
    stbi_set_flip_vertically_on_load(true);

    unsigned char* raw = stbi_load(path.string().c_str(),
                                   &tex_new->width_,
                                   &tex_new->height_,
                                   &tex_new->channels,
                                   STBI_rgb_alpha); // 强制补齐 alpha 通道。

    stbi_set_flip_vertically_on_load(false);

    if(!raw) {
      fmt::print(stderr, "\nTexture::loadFromFile - StbImage::{}\n\n", stbi_failure_reason());
      return nullptr;
    }

    tex_new->data_.reset(raw);
    tex_new->color_space_ = space;

    texture_cache_[path.string()] = tex_new;
    return tex_new;
  }

} // Medium