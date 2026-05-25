///
/// @file   AssetCache.h
/// @author Garfield949
/// @date   2026/3/3 23:36
/// @brief  
///
#pragma once

#include <filesystem>

#include "Forwards.h"

namespace tinyobj {
struct material_t;
}

namespace Medium {

enum class ColorSpace;

class AssetCache {
public:
  ~AssetCache() = default;

  static AssetCache& instance() {
    static AssetCache cache;
    return cache;
  }

  // 加载 Wavefront OBJ 模型文件
  MeshPtr loadFromObj (const std::filesystem::path& path_obj);

private:
  AssetCache() = default;

  void parseMTL(const MeshPtr &mesh, const std::vector<tinyobj::material_t>& tiny_materials,
                const std::filesystem::path& path_obj);

  TexturePtr loadTexture (const std::filesystem::path& filepath, const ColorSpace& color_space);

  // 使用 weak_ptr 进行观察，不增加引用计数
  std::unordered_map<std::string, std::weak_ptr<Mesh>>     mesh_cache_{};
  std::unordered_map<std::string, std::weak_ptr<Texture>>  texture_cache_{};
  std::unordered_map<std::string, std::weak_ptr<Material>> material_cache_{};
};

} // Medium