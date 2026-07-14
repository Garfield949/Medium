///
/// @file   RenderStatus.h
/// @author Garfield949
/// @date   2026/3/22 20:38
/// @brief
///
#pragma once

#include "Base/Types.h"

namespace Medium {

struct Statistics {
  void reset() {
    shadow_opaque_draw_calls = 0;
    scene_opaque_draw_calls  = 0;
    scene_transp_draw_calls  = 0;
    visible_polygons         = 0;
    written_pixels           = 0;
  }
  i32 totalDrawCalls() const {
    return shadow_opaque_draw_calls + 
           scene_opaque_draw_calls  + 
           scene_transp_draw_calls;
  }
  i32 shadow_opaque_draw_calls = 0;
  i32 scene_opaque_draw_calls  = 0;
  i32 scene_transp_draw_calls  = 0;
  i32 visible_polygons         = 0;
  i32 written_pixels           = 0;
};

struct RenderStatus {
  void reset() {
    statistics.reset();
  }
  Statistics statistics;
};

} // namespace Medium