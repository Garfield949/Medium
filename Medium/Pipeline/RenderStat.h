///
/// @file   RenderStat.h
/// @author Garfield949
/// @date   2026/3/22 20:38
/// @brief  该文件定义 Profiler 所需的每帧渲染开销状态 
///
#pragma once

#include "Base/Types.h"

namespace Medium {

struct RenderStat {
  u32 draw_calls = 0;

  void clear() noexcept {
    draw_calls = 0;
  }
};

} // namespace Medium