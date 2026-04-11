///
/// @file   MeshNode.cpp
/// @author Garfield949
/// @date   2026/2/13 22:58
/// @brief
///
#include "MeshNode.h"

#include "Resources/Mesh.h"

namespace Medium {
  
  const AABB3D MeshNode::aabb() const noexcept {
    return mesh_->aabb;
  }
  
  const MeshPtr& MeshNode::mesh() const noexcept {
    return mesh_;
  }

} // namespace Medium
