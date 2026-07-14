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
struct CameraContext;

/// @class RenderQueue
/// @brief 渲染队列，在每个 pass 收集渲染命令
class RenderQueue {
public:
  // 通过队列类型觉得排序方式。
  enum class Type {
    OpaqueQueue,
    TransparentQueue
  };

  explicit RenderQueue(Type type) : queue_type_{type} {}

  void clear() { cmds_.clear(); }
  
  void build(std::span<const MeshNode* const> mesh_nodes, 
             const CameraContext& cam_ctx);

  void sort();

  /// @brief 吐给 Pipeline::executeCommands
  std::span<const RenderCommand> commands() const noexcept {
    return cmds_;
  }

private:
  void sortOpaque();
  void sortTransparent();

  Type queue_type_;
  std::vector<RenderCommand> cmds_;
};

} // Medium