///
/// @file   AssetCache.cpp
/// @author Garfield949
/// @date   2026/3/3 23:36
/// @brief
///
#include "AssetCache.h"

#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#include <TinyObjLoader.h>
#include <stb_image.h>
#include <fmt/color.h>

#include "Base/Log.h"
#include "Base/Color.h"

#include "Mesh.h"
#include "SkyBox.h"
#include "CubeMap.h"
#include "Material.h"
#include "Texture2D.h"

namespace fs = std::filesystem;

namespace Medium {

namespace {

using namespace tinyobj;

// ---------- 用于 loadObj 函数里的 std::unordered_map: unique_map
struct IndexHash {
  size_t operator()(const index_t& idx) const noexcept {
    // {} 为初始化列表，这里是实例化一个计算 i32 的临时对象，() 调用了为 i32 重载的 operator().
    // 重载 operator() 后，那么对象可以像函数一样被调用：IndexHash a; size_t v = a(42);
    const size_t h1 = std::hash<i32>{}(idx.vertex_index);
    const size_t h2 = std::hash<i32>{}(idx.normal_index);
    const size_t h3 = std::hash<i32>{}(idx.texcoord_index);
    // boost::hash_combine 风格
    auto combine = [](size_t h, size_t v) {
      return h ^ (v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
    };
    size_t h = combine(h1, h2);
    h = combine(h, h3);
    return h;
  }
};

// ---------- 用于 loadObj 里的 std::unordered_map: unique_map
struct IndexEqual {
  bool operator()(const index_t& lhs, const index_t& rhs) const noexcept {
    // 仅索引相同还不够，因为同一个空间位置可能被多个面共用，而其在每个面上的法线和 uv 都可能不同，这样会被算作两个不同的顶点
    return lhs.vertex_index   == rhs.vertex_index &&
           lhs.normal_index   == rhs.normal_index &&
           lhs.texcoord_index == rhs.texcoord_index;
  }
};

// ---------- 提取 tinyobj::attrib_t 中的数据
Vec2f extractToVec2f(const std::vector<real_t>& data, const i32 idx) {
  if(idx < 0) return Vec2f::Zero(); // 法线或纹理不存在时，返回初始值零
  return { data[2 * idx], data[2 * idx + 1] };
}

// ---------- 提取 tinyobj::attrib_t 中的数据
Vec3f extractToVec3f(const std::vector<real_t>& data, const i32 idx) {
  if(idx < 0) return Vec3f::Zero();
  return { data[3 * idx], data[3 * idx + 1], data[3 * idx + 2] };
}

using FacesByMatMap = std::unordered_map<i32, std::vector<index_t>>;
FacesByMatMap getFacesByMatIDMap(const shape_t& shape) {
  // 将一个 shape 中材质相同的面分装在一起 [material_id] -> [索引vector]. vector 中每 3 个 index_t 为一个 face.
  FacesByMatMap map;
  // 遍历 shape 里的每个面。num_face_vertices 是一个 vector，num_face_vertices[i] 表示第 i 个
  // 面有几个顶点。由于已配置了三角化，这里只用到 num_face_vertices.size()，即获取总三角面片的个数。
  for(size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
    // 由面片的序号拿到对应的材质id. 如果面片没有材质，tinyobjloader 会将其 id 置 -1.
    i32 mat_id = shape.mesh.material_ids[f];
    // 无材质的面片一律装到默认材质的桶中 (parseMTL 阶段已将默认材质装入了 mesh_new->materials[0])
    if(mat_id < 0) {
      for(auto v = 0; v < 3; ++v)
        map[0].push_back(shape.mesh.indices[f * 3 + v]);
      continue;
    }
    // 含同一材质的三角面装在一起
    for(auto v = 0; v < 3; ++v)
      map[mat_id + 1].push_back(shape.mesh.indices[f * 3 + v]);
  }
  return map;
}

using VertByIndexMap = std::unordered_map<index_t, size_t, IndexHash, IndexEqual>;
void parseSubMeshesFromShapes(Mesh* mesh_new, const attrib_t& attrib, const std::vector<shape_t>& shapes) {
  // 负责存储 index_t -> 对应顶点在 mesh.vertices 中的下标。
  VertByIndexMap vert_pos_by_index_t;
  // 1. shape 为 obj 中 o 或 g 开头的面 f 分组，比如：
  //    g sponza_283
  //    usemtl fabric_d
  //    s 1
  //    f 67935/41200/64115 67936/41201/64116 67937/41202/64117 67938/41203/64118
  //    f 67939/41204/64119 67938/41203/64118 67937/41202/64117 67940/41205/64120
  //    f ...
  // 2. 注意大多 shape 仅对应一个材质，但也有部分 shape 会在 中途切换材质，比如 (拿 crytek sponza 举例):
  //    g sponza_01 为地上其中一个花瓶，其下部为花瓶身对应 newmtl vase_round，上部的绿植部分切换为 newmtl Material__57.
  // 3. 因此在组装 SubMesh 时，若 shape 只对应一个材质，则将其整体转为一个 SubMesh 即可；若单 shape 对应多个材质，则需拆分
  //    为若干 SubMesh 并指定相应材质。
  for(const auto& shape: shapes) {
    // 将单 shape 中材质相同的面组装为一个 SubMesh.
    for(const auto& [mat_id, face_indices]: getFacesByMatIDMap(shape)) {
      SubMesh submesh;
      submesh.index_offset   = mesh_new->indices.size();
      submesh.index_count    = face_indices.size();
      submesh.material_index = mat_id;
      // 将每个 SubMesh 顶点去重并计算顶点索引，并更新 index_map
      for(const index_t& index : face_indices) {
        auto iter = vert_pos_by_index_t.find(index);
        if(iter == vert_pos_by_index_t.end()) {
          // 原地构造一个 Vertex 对象。
          mesh_new->vertices.emplace_back(extractToVec3f(attrib.vertices,  index.vertex_index),
                                          extractToVec3f(attrib.normals,   index.normal_index),
                                          extractToVec2f(attrib.texcoords, index.texcoord_index));

          size_t new_vert_pos = mesh_new->vertices.size() - 1;  // 获得最新存入顶点的下标（即 vector 尾部）
          vert_pos_by_index_t[index] = new_vert_pos;            // unordered_map 存入键值对
          mesh_new->indices.push_back(new_vert_pos);            // Mesh 的索引数组存入对应顶点在 mesh_new.vertices 中的下标
        } else
          mesh_new->indices.push_back(iter->second);              // 已存在则直接存入对应顶点在 mesh_new.vertices 中的下标
      }
      mesh_new->sub_meshes.push_back(submesh);
    }
  }
}

void meshPostProcess(Mesh* mesh_new, bool no_normals) {
  // 若模型不自带法线就手动计算。
  if(no_normals) {
    Log::printLogPrefix("AssetCache", "loadFromObj", false);
    fmt::print("No normals detected in [{}], Vertex normals will be generated automatically.\n\n", mesh_new->name);
    mesh_new->calcNormals();
  }
  // 只要有任何一个 SubMesh 的材质启用了法线贴图，就计算整个网格的顶点切线
  for(const auto& mat: mesh_new->materials)
    if(mat.enable_norm) {
      mesh_new->calcTangents();
      break;
    }
  // 计算网格的包围盒
  mesh_new->calcAABBs();
}

} // ===---------- 匿名命名空间，控制仅单文件可见 ----------===

AssetCache::AssetCache()  = default;
AssetCache::~AssetCache() = default;

const Mesh* AssetCache::loadFromObj(const fs::path& obj_path) {
  // 1. lexically_normal() 将类似 home/user/../docs/./notes.txt 的路径转为 home/docs/notes.txt,
  //    并根据平台自动转换所有间隔符，如 windows 为 '\' , macos 为 '/';
  // 2. generic_string 将 path 转为字符串的同时，强制将间隔符转为 windows 与类 unix 环境通用的 '/'.
  const auto  obj_path_norm = obj_path.lexically_normal();
  const auto  obj_path_str  = obj_path_norm.generic_string();
  const auto& mesh_key      = obj_path_str;

  // 若迭代器 == mesh_cache_.end()，说明并不存在 key 对应的值。
  if(auto iter = mesh_cache_.find(mesh_key); iter != mesh_cache_.end()) {
    Log::printLogPrefix("AssetCache", "loadFromObj", false);
    fmt::print("[{}] cached\n", obj_path_str);
    Mesh* mesh_cached = iter->second.get();
    return mesh_cached;
  }

  ObjReaderConfig config;
  // .mtl 与 .obj 于同一文件夹下
  config.mtl_search_path = obj_path_norm.parent_path().generic_string();
  config.triangulate = true;

  ObjReader reader;
  if(!reader.ParseFromFile(obj_path_norm, config)) {
    if(!reader.Error().empty()){
      Log::printLogPrefix("AssetCache", "loadFromObj", true);
      fmt::print("TinyObjReader::{}\n", reader.Error());
    }    
    return nullptr;
  }

  // 1. attrib_t 为一个含多个 std::vector 的结构体，存储实际的顶点数据，如坐标、法线、纹理等。顶点数组
  //    std::vector<real_t> vertices 存放的是平铺的分量数据，即 x0, y0, z0, x1, y1, z1,... 法线
  //    和纹理同理；
  // 2. shape_t 有 Mesh、Lines、Points。但大多数模型都只以 Mesh 来表示，其中 Mesh 类型存放顶点索引；
  // 3. 一个模型可能有多个 shape。比如头、身、四肢，每个 shape 有独立的 mesh，所以 shapes 的大小有可能
  //    是一；在 .obj 文件中，每一个 o 或 g 开头的 f 面片分组会被识别为一个 shape;
  // 4. materials 由识别 .mtl 得到，每个材质为 mtl 里 "usemtl xxx" 开头的分组。
  const attrib_t&                attrib    = reader.GetAttrib();
  const std::vector<shape_t>&    shapes    = reader.GetShapes();
  const std::vector<material_t>& materials = reader.GetMaterials();

  Log::printLogPrefix("\nAssetCache", "loadFromObj", false);
  fmt::print("[{}]\n", obj_path_str);
  fmt::print("Vertices : {}\n"
             "Normals  : {}\n"
             "UVs      : {}\n"
             "Submeshes: {}\n"
             "Materials: {}\n\n", attrib.vertices.size()/3, attrib.normals.size()/3, attrib.texcoords.size()/2,
                                  shapes.size(), materials.size());

  // 顶点、索引数组空间预计算。
  size_t total_indices = 0;
  for(const auto& shape: shapes)
    total_indices += shape.mesh.indices.size();

  // 创建一个 Mesh 强缓存。
  std::unique_ptr<Mesh> mesh_owned = std::make_unique<Mesh>();
  mesh_owned->name = obj_path_norm.filename();
  mesh_owned->indices.reserve(total_indices);
  mesh_owned->vertices.reserve(total_indices / 3); // 顶点数一般是索引数的 1/3，参考一个正方体。

  // 解析 MTL，先将所有材质从前到后灌入 Mesh->materials
  parseMTL(mesh_owned.get(), materials, obj_path_norm);
  // 将材质相同的面组为一个子网格，并进行顶点去重生成 Vertex 和顶点索引数组存入 Mesh
  parseSubMeshesFromShapes(mesh_owned.get(), attrib, shapes);
  // 依据情况计算 Mesh 的顶点法线、切线以及包围盒
  meshPostProcess(mesh_owned.get(), attrib.normals.empty());

  // emplace 返回一个 std::pair<iterator, bool>，其中 bool 指示 key 是否已存在。
  // std::unique_ptr 禁止拷贝，只能移动。
  auto iter = mesh_cache_.emplace(mesh_key, std::move(mesh_owned)).first;
  return iter->second.get();
}

SkyBox* AssetCache::loadSkyBox(const std::filesystem::path& cubemap_img_path) {
  const std::string img_path_str = cubemap_img_path.generic_string();
  auto& img_key = img_path_str;

  if(auto iter = skybox_cache_.find(img_key); iter != skybox_cache_.end()) {
    SkyBox* skybox_cached = iter->second.get();
    Log::printLogPrefix("AssetCache", "loadHorizontalCrossSkybox", false);
    fmt::print("[{}] cached\n", img_path_str);
    return skybox_cached;
  }

  i32 width, height, channels;
  // 翻转十字 cubemap 后，top 和 bottom 的位置会互换。
  stbi_set_flip_vertically_on_load(true);
  unsigned char* raw = stbi_load(img_path_str.c_str(),
                                 &width,
                                 &height,
                                 &channels,
                                 STBI_rgb_alpha); // 强制补齐 alpha 通道。
  if(!raw) {
    Log::printLogPrefix("AssetCache", "loadHorizontalCrossSkybox", true);
    fmt::print("StbImage::{}\n\n", stbi_failure_reason());
    return nullptr;
  }

  Image2D<Color> img_base{width, height};

  std::memcpy(img_base.data(), raw, img_base.pixelCount() * sizeof(Color));
  stbi_image_free(raw);

  auto skybox_owned     = std::make_unique<SkyBox>();
  skybox_owned->cubemap = CubeMap::FromHorizontalCross(img_base);
  skybox_owned->env_sh9 = EnvironmentSH9::FromCubeMap(skybox_owned->cubemap);

  // emplace 返回一个 std::pair<iterator, bool>，其中 bool 指示 key 是否已存在。
  // std::unique_ptr 禁止拷贝，只能移动。
  auto iter = skybox_cache_.emplace(img_key, std::move(skybox_owned)).first;
  return iter->second.get();
}

void AssetCache::parseMTL(Mesh* mesh_new,
                          const std::vector<material_t>& tiny_materials,
                          const std::filesystem::path& obj_path)
{
  // Mesh 的材质数组第一个位置放置默认材质（绿色纯色）
  mesh_new->materials.emplace_back();

  for(const auto& mat_t: tiny_materials) {
    Material mat_new{mat_t.name};

    // 将 TinyObjLoader 读取的原始材质参数装入自定义结构。
    // 总是认为 MTL 里定义的 Kd 漫反射纯色是一个归一化的 Srgb 值，虽然各种模型都不太规范就是了。
    MaterialParams& mat_params = mat_new.params;
    mat_params.Kd    = LinearColor::FromNormalizedSRGB(mat_t.diffuse[0],  mat_t.diffuse[1],  mat_t.diffuse[2], 1.0f);
    mat_params.Ks    = LinearColor{mat_t.specular[0], mat_t.specular[1], mat_t.specular[2]};
    mat_params.Ns    = mat_t.shininess;
    mat_params.d     = mat_t.dissolve;
    mat_params.illum = mat_t.illum;

    auto fetchTexPtr = [&](const fs::path& tex_path, ColorSpace space) -> Texture2D* {
      // 材质的某些通道（如漫反射）可能是不含材质图片的纯色，直接返回对应的空贴图指针即可；用 loadTexture 返回 nullptr 也行，但
      // 其会输出非法路径的错误信息，与语义不合。
      if(tex_path.empty()) return nullptr;
      // 若 tex_path 开头有 / 或 \ ，std::filesystem::path 会认为它是一个绝对路径，进而将 obj_path.parent_path() 丢掉，
      // 所以这里总是将其转为相对路径。
      fs::path full_path = (obj_path.parent_path() / tex_path.relative_path()).lexically_normal();
      return loadTexture(full_path, space);
    };

    mat_new.diff_slot = makeSlot(fetchTexPtr(mat_t.diffuse_texname,  ColorSpace::SRGB),   mat_params.Kd);
    mat_new.spec_slot = makeSlot(fetchTexPtr(mat_t.specular_texname, ColorSpace::Linear), mat_params.Ks);
    mat_new.diss_slot = fetchTexPtr(mat_t.alpha_texname,  ColorSpace::Linear);
    mat_new.norm_slot = fetchTexPtr(mat_t.normal_texname, ColorSpace::Linear);

    mat_new.setAttributes();

    mesh_new->materials.push_back(mat_new);
  }
}

Texture2D* AssetCache::loadTexture(const fs::path& tex_path, const ColorSpace& color_space) {
  const std::string tex_path_str = tex_path.generic_string();
  // C++23 小工具 std::to_underlying, 将枚举值转换为其底层整型值。
  const std::string tex_key = fmt::format("{}#colorspace={}", tex_path_str, std::to_underlying(color_space));

  if(auto iter = texture_cache_.find(tex_key); iter != texture_cache_.end()) {
    Texture2D* tex_cached = iter->second.get();
    Log::printLogPrefix("AssetCache", "loadTexture", false);
    fmt::print("[{}] cached\n", tex_path_str);
    return tex_cached;
  }

  // 纹理图的存储默认原点在左上角。
  i32 width, height, channels;
  stbi_set_flip_vertically_on_load(true);
  unsigned char* raw = stbi_load(tex_path_str.c_str(),
                                 &width,
                                 &height,
                                 &channels,
                                 STBI_rgb_alpha); // 强制补齐 alpha 通道。

  if(!raw) {
    Log::printLogPrefix("AssetCache", "loadTexture", true);
    fmt::print("StbImage::{}\n\n", stbi_failure_reason());
    return nullptr;
  }

  Image2D<Color> base{width, height};

  std::memcpy(base.data(), raw, base.pixelCount() * sizeof(Color));
  stbi_image_free(raw);

  // Image2D 显示删除了拷贝构造，必须用 move.
  auto texture_owned = std::make_unique<Texture2D>(std::move(base), color_space);
  texture_owned->generateMips();

  // emplace 返回一个 std::pair<iterator, bool>，其中 bool 指示 key 是否已存在。
  // std::unique_ptr 禁止拷贝，只能移动。
  auto iter = texture_cache_.emplace(tex_key, std::move(texture_owned)).first;
  return iter->second.get();
}

} // namespace Medium
