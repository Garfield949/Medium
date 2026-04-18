///
/// @file   MeshNode.h
/// @author Garfield949
/// @date   2026/2/12 22:26
/// @brief  
///
#pragma once

#include "Scene/Node.h"
#include "Geometry/AABB.h"
#include "Resources/Forwards.h"

namespace Medium {
  
class RenderQueue;

class MeshNode : public Node {
public:
  static constexpr NodeType Type = NodeType::MeshInstance;

  explicit MeshNode(std::string name, MeshPtr mesh)
    : Node{std::move(name), Type}, mesh_{std::move(mesh)} {}

  const AABB3D aabb() const noexcept;
  
  const MeshPtr& mesh() const noexcept;

private:
  MeshPtr mesh_;
};

} // namespace Medium
