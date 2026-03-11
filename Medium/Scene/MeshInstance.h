///
/// @file   MeshInstance.h
/// @author Garfield949
/// @date   2026/2/12 22:26
/// @brief  
///
#pragma once

#include "Scene/Node.h"
#include "Resources/Forwards.h"

namespace Medium {

  class MeshInstance : public Node {
  public:
    explicit MeshInstance(std::string name, MeshPtr mesh)
      : Node(std::move(name)), mesh_(std::move(mesh)) {}

    void render(const RenderContext& context) override;

  private:
    MeshPtr mesh_;
  };

} // namespace Medium
