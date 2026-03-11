///
/// @file   Node.h
/// @author Garfield949
/// @date   2026/2/11 22:02
/// @brief  
///
#pragma once

#include "Transform.h"
#include "Pipeline/RenderContext.h"

namespace Medium {

  class Node {
    friend class Transform;
  public:
    explicit Node(std::string name = "Node");
    virtual ~Node() = default;

    virtual void update(f32 dt);

    virtual void render(const RenderContext& context);

    template<typename T>
    T* attachChild(std::unique_ptr<T> child) {
      static_assert(std::is_base_of<Node, T>::value, "T must derive from Node");
      assert(child && "Cannot attach a null child.");

      T *ptr = child.get();

      child->parent_ = this;
      child->transform.updateDirtyFlags();
      children_.push_back(std::move(child));

      return ptr;
    }

    void setName(std::string name) { name_ = std::move(name); }

    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    Transform transform;

  protected:
    std::string name_;
    Node *parent_;
    std::vector<std::unique_ptr<Node>> children_;
  };

} // Medium