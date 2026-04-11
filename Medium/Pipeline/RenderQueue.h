///
/// @file   RenderQueue.h
/// @author Garfield949
/// @date   2026/3/19 10:33
/// @brief  该文件定义渲染队列类型
///
#pragma once

#include "RenderCommand.h"

namespace Medium {
  
  class Node;
  struct ViewCtx;

  /// @class RenderQueue
  /// @brief 渲染队列，在每个 pass 收集渲染命令
  class RenderQueue {
  public:
    enum class Type {
      Shadow,
      Opaque,
      Transparent,
    };
  
    RenderQueue(Type type) : type_{type} {}
    
    void build(const Node* scene_root, const ViewCtx& view_ctx);

    void sort();
    
    [[nodiscard]] const std::vector<RenderCommand>& commands() const noexcept {
      return cmds_;
    }

  private:
    void sortShadow();
    void sortOpaque();
    void sortTransparent();
  
    Type type_;
    std::vector<RenderCommand> cmds_;
  };

} // Medium