///
/// @file   MeshNode.h
/// @author Garfield949
/// @date   2026/2/12 22:26
/// @brief  
///
#pragma once

#include "Base/Geometry/AABB3D.h"
#include "Scene/Node.h"
#include "Resources/Mesh.h"

namespace Medium {
  
class RenderQueue;

class MeshNode : public Node {
public:
  static constexpr NodeType Type = NodeType::MeshInstance;

  explicit MeshNode(std::string name, const Mesh* mesh)
    : Node{std::move(name), Type}, mesh_{mesh} {}

  AABB3D localAABB() const noexcept;

  AABB3D worldAABB() const noexcept;
  
  const Mesh* mesh() const noexcept;

private:
  const Mesh* mesh_ = nullptr;
};

} // namespace Medium
