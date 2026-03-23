///
/// @file Mesh.cpp
///
#include "Mesh.h"

#include <iostream>
#include <fmt/base.h>

namespace Medium {

  // 面积加权平均法线 Area-weighted vertex normals. 对于每个三角面片，计算其法向，并累加到三个顶点的法向上。
  // 最后将所有顶点的法向归一化。
  void Mesh::calcNormals() noexcept {
    for(size_t f = 0; f < indices.size(); f += 3) {
      Vec3f& pos0 = vertices[indices[f + 0]].pos;
      Vec3f& pos1 = vertices[indices[f + 1]].pos;
      Vec3f& pos2 = vertices[indices[f + 2]].pos;

      Vec3f face_normal = (pos1 - pos0).cross(pos2 - pos0);

      for(size_t j = 0; j < 3; ++j)
        vertices[indices[f + j]].normal += face_normal;
    }
    for(auto & vertex : vertices) vertex.normal.normalize();
  }
  
  // 依据 3D Math Primer 的推导实现
  void Mesh::calcTangents() noexcept {
    size_t verts_num = vertices.size();

    std::vector<Vec3f> tmp_tangents  (verts_num, Vec3f::Zero());
    std::vector<Vec3f> tmp_bitangents(verts_num, Vec3f::Zero());

    // 逐面遍历
    for(size_t f = 0; f < indices.size(); f += 3) {
      Vec3f& pos0 = vertices[indices[f + 0]].pos;
      Vec3f& pos1 = vertices[indices[f + 1]].pos;
      Vec3f& pos2 = vertices[indices[f + 2]].pos;

      Vec2f& uv0 = vertices[indices[f + 0]].uv;
      Vec2f& uv1 = vertices[indices[f + 1]].uv;
      Vec2f& uv2 = vertices[indices[f + 2]].uv;

      Vec3f delta_pos01 = pos1 - pos0;
      Vec3f delta_pos02 = pos2 - pos0;

      f32 delta_u01 = uv1.x() - uv0.x();
      f32 delta_u02 = uv2.x() - uv0.x();
      f32 delta_v01 = uv1.y() - uv0.y();
      f32 delta_v02 = uv2.y() - uv0.y();

      f32 sign = (delta_u01 * delta_v02 - delta_u02 * delta_v01) < 0 ? -1.0f : 1.0f;

      // 计算三角面层级的 T 和 B
      Vec3f tangent_f   =  (delta_v02 * delta_pos01 - delta_v01 * delta_pos02) * sign;
      Vec3f bitangent_f = (-delta_u02 * delta_pos01 + delta_u01 * delta_pos02) * sign;

      // 累加到顶点的 T 和 B
      for(size_t j = 0; j < 3; ++j) {
        tmp_tangents[indices[f + j]]   += tangent_f;
        tmp_bitangents[indices[f + j]] += bitangent_f;
      }
    }

    // 逐顶点矫正
    for(size_t i = 0; i < vertices.size(); ++i) {
      Vertex& vertex = vertices[i];
      Vec3f T = tmp_tangents[i];
      const Vec3f& B = tmp_bitangents[i];

      // 施密特正交化
      T = (T - (T.dot(vertex.normal) * vertex.normal)).normalized();

      f32 det = 1.0f;
      if(T.cross(vertex.normal).dot(B) < 0)
        det = -1.0f;

      vertex.tangent.head<3>() = T;
      vertex.tangent.w() = det;
    }
  }

  void Mesh::calcAABBs() noexcept {
    // 在整个 Mesh 上算 AABB
    // TODO: 先算 SubMesh 的 AABB，再 merge 为整体的 AABB
    for(const auto & v : vertices) {
      bbox.merge(v.pos);
    }
    // 在 SubMesh 层级算 AABB
    for(auto & sub_mesh : sub_meshes) {
      size_t offset = sub_mesh.index_offset;
      size_t count  = sub_mesh.index_count;
      for(size_t i = offset; i < offset + count; ++i) {
        sub_mesh.bbox.merge(vertices[indices[i]].pos);
      }
    }
  }

} // namespace Medium