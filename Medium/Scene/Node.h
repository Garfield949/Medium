///
/// @file   Node.h
/// @author Garfield949
/// @date   2026/2/11 22:02
/// @brief
///
#pragma once

#include "Transform.h"

namespace Medium {

class DrawQueue;
struct CameraContext;

// 用于 cast_to 进行类型识别
enum class NodeType { Vanilla, Mesh, Light, Camera };

class Node {
  friend class Transform;
public:
  NodeType  node_type;
  Transform transform;

  explicit Node(std::string name = "Node", NodeType type = NodeType::Vanilla)
    : node_type{type}, transform{this}, name_{std::move(name)} {}

  virtual ~Node() = default;

  void setName(std::string name) { name_ = std::move(name); }

  const std::string& name() const noexcept { return name_; }

  /// @brief 静态方法，试图将传入的 Node* 静态转型为指定的 NodeT*
  template<typename NodeT>
  static NodeT* CastTo(Node* node) {
    if(node->node_type == NodeT::Type) return static_cast<NodeT*>(node);
    return nullptr;
  }

  /// @brief cast_to 的 const 重载
  template<typename NodeT>
  static const NodeT* CastTo(const Node* node) {
    if(node->node_type == NodeT::Type) return static_cast<const NodeT*>(node);
    return nullptr;
  }

  /// @brief 遍历子节点(DFS)
  /// @param f 传入一个可调用对象，一般是 Lambda.
  template<typename Callable>
  void traverse(Callable&& f) {
    f(this);
    for(auto& child: children_) {
      child->traverse(f);
    }
  }

  /// @brief traverse 的 const 重载
  template<typename Callable>
  void traverse(Callable&& f) const {
    f(this);
    for(const auto& child: children_) {
      child->traverse(f);
    }
  }

  /// @brief 挂载一个新的子结点
  /// @param child 传入一个 Node 派生类型指针
  /// @return 返回挂载节点的裸观察指针
  template<typename NodeT>
  NodeT* attachChild(std::unique_ptr<NodeT>&& child) {
    static_assert(std::is_base_of_v<Node, NodeT>, "NodeT must derive from Node");
    assert(child && "Cannot attach a null child.");

    NodeT *ptr = child.get();

    child->parent_ = this;
    child->transform.updateDirtyFlags();
    children_.push_back(std::move(child));

    return ptr;
  }

protected:
  // TODO: 暂时不知道给节点起名有啥用。
  std::string name_;
  Node* parent_ = nullptr;
  std::vector<std::unique_ptr<Node>> children_;
};

} // Medium
