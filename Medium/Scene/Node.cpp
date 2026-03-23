///
/// @file   Node.cpp
/// @author Garfield949
/// @date   2026/2/11 22:02
/// @brief  
///
#include "Node.h"

namespace Medium {

  Node::Node(std::string name) : transform(this),
                                 name_(std::move(name)),
                                 parent_(nullptr) {}

  void Node::update(f32 dt) {}
  
  void Node::commitCommand(RenderQueue& queue, const Frustum& frustum) const {
    for(auto& child : children_) {
      child->commitCommand(queue, frustum);
    }
  }

} // Medium