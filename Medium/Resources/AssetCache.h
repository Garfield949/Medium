///
/// @file   AssetCache.h
/// @author Garfield949
/// @date   2026/3/3 23:36
/// @brief  
///
#pragma once

#include <vector>
#include <filesystem>
#include <unordered_map>

#include "Material.h"

namespace tinyobj {
struct material_t;
}

namespace Medium {

enum class ColorSpace;
class Mesh;
class Texture2D;
class CubeMap;

class AssetCache {
public:
  // Mesh 是前向声明，而 unique_ptr 的删除器要求 Mesh 是个完整类型。在头文件隐式生成的析构函数看不到 Mesh 的完
  // 整定义，clangd 会报错。解决方案是将构造函数和析构函数的定义放在 cpp，在那里包含 Mesh.h 析构函数就能看到了。
  AssetCache();
  ~AssetCache();

  /// @brief 获取单例。
  static AssetCache& instance() {
    static AssetCache cache;
    return cache;
  }

  // 加载 Wavefront OBJ 模型文件。
  [[nodiscard]] const Mesh* loadFromObj (const std::filesystem::path& obj_path);

  // 加载天空盒 cubemap 图片。
  [[nodiscard]] CubeMap* loadHorizontalCrossSkybox(const std::filesystem::path& img_path);

private:
  void parseMTL(Mesh* mesh_new,
                const std::vector<tinyobj::material_t>& tiny_materials,
                const std::filesystem::path& obj_path);

  Texture2D* loadTexture(const std::filesystem::path& filepath, const ColorSpace& color_space);

  std::unordered_map<std::string, std::unique_ptr<Mesh>>      mesh_cache_;
  std::unordered_map<std::string, std::unique_ptr<Texture2D>> texture_cache_;
  std::unordered_map<std::string, std::unique_ptr<CubeMap>>   skybox_cache_;
};

} // Medium