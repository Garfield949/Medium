///
/// @file   RenderStat.h
/// @author Garfield949
/// @date   2026/3/22 20:38
/// @brief
///
#pragma once

#include "Base/Types.h"
#include "Pipeline/Shadow/CSM.h"

namespace Medium {

struct Statistics {
  void reset() {
    shadow_opaque_draw_calls = 0;
    scene_opaque_draw_calls  = 0;
    scene_transp_draw_calls  = 0;
    visible_triangles        = 0;
    written_pixels           = 0;
  }
  u32 totalDrawCalls() const {
    return shadow_opaque_draw_calls + 
           scene_opaque_draw_calls  + 
           scene_transp_draw_calls;
  }
  u32 shadow_opaque_draw_calls = 0;
  u32 scene_opaque_draw_calls  = 0;
  u32 scene_transp_draw_calls  = 0;
  u32 visible_triangles        = 0;
  u32 written_pixels           = 0;
};

struct CSM {
  f32 max_shadow_distance = 0.0f;
  u32 cascade_count = 0;
  std::array<f32, kMaxCSMCascades + 1> csm_slices;
};

struct RenderStat {
  void reset() {
    statistics.reset();
  }
  Statistics statistics;
  CSM csm;
};

} // namespace Medium