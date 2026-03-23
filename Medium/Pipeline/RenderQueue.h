///
/// @file   RenderQueue.h
/// @author Garfield949
/// @date   2026/3/19 10:33
/// @brief  
///
#pragma once

#include "RenderCommand.h"

namespace Medium {

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
          return a.matrix_world->col(3).z() < b.matrix_world->col(3).z();
      });
    }
    
    [[nodiscard]] const std::vector<RenderCommand>& opaqueQueue() const noexcept {
      return opaque_queue_;
    }

  private:
    std::vector<RenderCommand> opaque_queue_;
    std::vector<RenderCommand> shadow_caster_queue_;
  };

} // Medium