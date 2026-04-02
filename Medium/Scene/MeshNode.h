///
/// @file   MeshNode.h
/// @author Garfield949
/// @date   2026/2/12 22:26
/// @brief  
///
#pragma once

#include "Scene/Node.h"
#include "Geometry/AABB.h"
#include "Resources/Mesh.h"

namespace Medium {
  
  class RenderQueue;

  class MeshNode : public Node {
  public:
    static constexpr NodeType Type = NodeType::Mesh;

    explicit MeshNode(std::string name, MeshPtr mesh)
      : Node{std::move(name), Type}, mesh_{std::move(mesh)} {}

    AABB3D aabb() const noexcept {
      return mesh_->aabb;
    }
    
    void commitCommand(RenderQueue& queue, const ViewCtx& view_ctx) const override;

  private:
    MeshPtr mesh_;
  };

} // namespace Medium
