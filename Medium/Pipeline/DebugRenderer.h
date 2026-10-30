///
/// @file   DebugRenderer.h
/// @author Garfield949
/// @date   2026/7/3 23:30
/// @brief  
///
#pragma once

#include "Base/Types.h"

namespace Medium {

class FrameBuffer;
struct DrawCommand;
struct RenderStatus;
struct SceneDrawListView;

class DebugRenderer {
public:
  explicit DebugRenderer() = default;

  static void drawAABBs(Mat4f mat_vp,
                        const SceneDrawListView& draw_list,
                        FrameBuffer&  frame_buffer,
                        RenderStatus& render_status);
};

} // namespace Medium
