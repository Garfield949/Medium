///
/// @file   RenderQueue.h
/// @author Garfield949
/// @date   2026/3/19 10:33
/// @brief  该文件定义渲染队列类型
///
#pragma once

#include "RenderCommand.h"

namespace Medium {

  /// @class RenderQueue
  /// @brief 渲染队列，在每个 pass 收集渲染命令
  class RenderQueue {
  public:
    RenderQueue() = default;

    void collectCommands(const RenderCommand& cmd) {
      opaque_queue_.push_back(cmd);
    }
    
    void clear() {
      opaque_queue_.clear();
    }

    void sort() {
      std::sort(opaque_queue_.begin(), opaque_queue_.end(),
        [](const RenderCommand& a, const RenderCommand& b) {
          return a.sort_key > b.sort_key;
      });
    }
    
    [[nodiscard]] const std::vector<RenderCommand>& opaqueQueue() const noexcept {
      return opaque_queue_;
    }

  private:
    std::vector<RenderCommand> shadow_caster_queue_;
    std::vector<RenderCommand> opaque_queue_;
    std::vector<RenderCommand> transparent_queue_;
  };

} // Medium