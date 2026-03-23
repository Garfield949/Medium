///
/// @file   RenderStat.h
/// @author Garfield949
/// @date   2026/3/22 20:38
/// @brief  
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