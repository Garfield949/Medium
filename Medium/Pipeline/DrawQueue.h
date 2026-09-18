///
/// @file   DrawQueue.h
/// @author Garfield949
/// @date   2026/3/19 10:33
/// @brief  该文件定义绘制队列类型
///
#pragma once

#include <span>

#include "DrawCommand.h"

namespace Medium {

class MeshNode;
struct CameraContext;

/// @class DrawQueue
/// @brief 绘制队列，在每个 pass 收集渲染命令
class DrawQueue {
public:
  // 通过队列类型觉得排序方式。
  enum class Type {
    Opaque,
    Transparent
  };

  explicit DrawQueue(Type type) : queue_type_{type} {}

  void clear() { cmds_.clear(); }
  
  void build(std::span<const MeshNode* const> mesh_nodes, 
             const CameraContext& cam_ctx);

  void sort();

  /// @brief 吐给 Pipeline::executeDrawCommands
  std::span<const DrawCommand> drawCommands() const noexcept {
    return cmds_;
  }

private:
  void sortOpaque();
  void sortTransparent();

  Type queue_type_;
  std::vector<DrawCommand> cmds_;
};

} // Medium