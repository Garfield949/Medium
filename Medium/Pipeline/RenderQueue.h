///
/// @file   RenderQueue.h
/// @author Garfield949
/// @date   2026/3/19 10:33
/// @brief  该文件定义渲染队列类型
///
#pragma once

#include <span>

#include "RenderCommand.h"

namespace Medium {

class MeshNode;
struct CameraCtx;

/// @class RenderQueue
/// @brief 渲染队列，在每个 pass 收集渲染命令
class RenderQueue {
public:
  enum class Type {
    OpaqueQueue,
    TransparentQueue
  };

  explicit RenderQueue(Type type) : type_{type} {}
  
  void build(std::span<const MeshNode* const> mesh_nodes, const CameraCtx& view_ctx);

  void sort();
  
  const std::vector<RenderCommand>& commands() const noexcept {
    return cmds_;
  }

private:
  void sortOpaque();
  void sortTransparent();

  Type type_;
  std::vector<RenderCommand> cmds_;
};

} // Medium