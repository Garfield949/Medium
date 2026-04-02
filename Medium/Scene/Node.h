///
/// @file   Node.h
/// @author Garfield949
/// @date   2026/2/11 22:02
/// @brief  
///
#pragma once

#include "Transform.h"

namespace Medium {

  class RenderQueue;
  struct ViewCtx;
  
  enum class NodeType { Vanilla, Mesh, Light, Camera };

  class Node {
    friend class Transform;
  public:
    NodeType  node_type;
    Transform transform;

    Node(std::string name = "Node", NodeType type = NodeType::Vanilla)
      : transform{this}, name_{std::move(name)}, node_type{type} {}

    virtual ~Node() = default;
    
    virtual void commitCommand(RenderQueue& queue, const ViewCtx& view_ctx) const {
      for(auto& child : children_) {
        child->commitCommand(queue, view_ctx);
      }
    }

    void setName(std::string name) { name_ = std::move(name); }

    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    /// @brief 静态方法，试图将传入的 Node* 静态转型为指定的 NodeT*
    template<typename NodeT>
    static NodeT* cast_to(Node* node) {
      if(node->node_type == NodeT::Type)
        return static_cast<NodeT*>(node);
      return nullptr;
    }

    /// @brief 遍历子节点，DFS
    template<typename Callable>
    void traverse(Callable&& f) {
      f(this);
      for(auto& child : children_) {
        child->traverse(std::forward<Callable>(f));
      }
    }

    template<typename NodeT>
    NodeT* attachChild(std::unique_ptr<NodeT>&& child) {
      static_assert(std::is_base_of<Node, NodeT>::value, "NodeT must derive from Node");
      assert(child && "Cannot attach a null child.");

      NodeT *ptr = child.get();

      child->parent_ = this;
      child->transform.updateDirtyFlags();
      children_.push_back(std::move(child));

      return ptr;
    }

  protected:
    std::string name_;
    Node* parent_ = nullptr;
    std::vector<std::unique_ptr<Node>> children_;
  };

} // Medium