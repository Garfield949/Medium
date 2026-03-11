///
/// @file   Node.cpp
/// @author Garfield949
/// @date   2026/2/11 22:02
/// @brief  
///
#include "Node.h"

#include <utility>

namespace Medium {

  Node::Node(std::string name) : transform(this),
                   name_(std::move(name)),
                   parent_(nullptr) {}

  void Node::update(f32 dt) {}

  // 默认行为：只递归渲染子节点
  void Node::render(const RenderContext &context) {
    for(auto& child : children_) {
      child->render(context);
    }
  }

} // Medium