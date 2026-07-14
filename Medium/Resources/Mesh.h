///
/// @file Mesh.h
/// @brief 网格类型
///
#pragma once

#include "Base/Geometry/AABB3D.h"
#include "Resources/Vertex.h"
#include "Resources/Material.h"

namespace Medium {

struct SubMesh {
  size_t index_offset    = 0;
  size_t index_count     = 0;
  size_t material_index  = 0;
  // 保存子网格的本地轴对齐包围盒，在读取模型时完成计算。
  AABB3D aabb_local;
};

/// @class Mesh
/// @brief 网格资源类型
/// @details 存储网格顶点和索引，图元为三角形
class Mesh {
  friend class AssetCache;
public:
  explicit Mesh() = default;

  std::string name;

  void calcNormals()  noexcept;
  void calcTangents() noexcept;
  void calcAABBs()    noexcept;

  std::vector<SubMesh>  sub_meshes; // 子网格数据
  std::vector<Material> materials;  // 子网格材质

  std::vector<Vertex> vertices; // 去重后的顶点数据
  std::vector<size_t> indices;  // 索引，存的是对应顶点在 vertices 里的下标

  AABB3D aabb_local;
};

} // namespace