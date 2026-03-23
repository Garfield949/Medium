///
/// @file   Node.h
/// @author Garfield949
/// @date   2026/2/11 22:02
/// @brief  
///
#pragma once

#include "Geometry/Frustum.h"
#include "Transform.h"

namespace Medium {

  class RenderQueue;
  class Frustum;

  class Node {
    friend class Transform;
  public:
    Node(std::string name = "Node");

    Node(Node&&) noexcept = default;
    Node& operator=(Node&& other) noexcept = default;

    virtual ~Node() = default;

    virtual void update(f32 dt);
    
    virtual void commitCommand(RenderQueue& queue, const Frustum& frustum) const;

    void setName(std::string name) { name_ = std::move(name); }
    
    void setParent(Node* parent) { parent_ = parent; }

    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    template<typename T>
    T* attachChild(std::unique_ptr<T>&& child) {
      static_assert(std::is_base_of<Node, T>::value, "T must derive from Node");
      assert(child && "Cannot attach a null child.");

      T *ptr = child.get();

      child->parent_ = this;
      child->transform.updateDirtyFlags();
      children_.push_back(std::move(child));

      return ptr;
    }

    Transform transform;

  protected:
    std::string name_;
    Node* parent_;
    std::vector<std::unique_ptr<Node>> children_;
  };

} // Medium