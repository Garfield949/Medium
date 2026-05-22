///
/// @file   MeshNode.cpp
/// @author Garfield949
/// @date   2026/2/13 22:58
/// @brief
///
#include "MeshNode.h"

#include "Resources/Mesh.h"

namespace Medium {
  
AABB3D MeshNode::localAABB() const noexcept {
  return mesh_->aabb_local;
}

AABB3D MeshNode::worldAABB() const noexcept {
  return mesh_->aabb_local.toWorldAABB(
    this->transform.worldMatrix()
  );
}

const MeshPtr& MeshNode::mesh() const noexcept {
  return mesh_;
}

} // namespace Medium
