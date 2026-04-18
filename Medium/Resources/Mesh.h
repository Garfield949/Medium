///
/// @file Mesh.h
/// @brief 网格类型
///
#pragma once

#include "Forwards.h"
#include "Vertex.h"
#include "Geometry/AABB.h"

namespace Medium {

struct SubMesh {
  [[nodiscard]] size_t triangleCnt() const noexcept {
    return index_count / 3;
  }

  size_t index_offset    = 0;
  size_t index_count     = 0;
  i32    material_index  = 0;

  AABB3D bbox;
};

/// @class Mesh
/// @brief 网格资源类型
/// @details 存储网格顶点和索引，图元为三角形
class Mesh {
  friend class AssetCache;
public:
  ~Mesh() = default;

  void calcNormals()  noexcept;
  void calcTangents() noexcept;
  void calcAABBs()    noexcept;
  
  std::string name;

  std::vector<SubMesh>     sub_meshes; // 子网格数据
  std::vector<MaterialPtr> materials;  // 子网格材质

  std::vector<Vertex> vertices; // 去重后的顶点数据
  std::vector<u32>    indices;  // 索引，存的是对应顶点在 vertices 里的下标

  AABB3D aabb;
private:
  Mesh() = default;
};

} // namespace