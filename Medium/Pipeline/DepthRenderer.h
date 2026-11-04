///
/// @file   DepthRenderer.h
/// @author Garfield949
/// @date   2026/10/31 10:15
/// @brief  
///
#pragma once

#include "Pipeline/RenderContext.h"
#include "Pipeline/Shader/ShaderVariant.h"

namespace Medium {

class FrameBuffer;
struct RenderStatus;
struct SceneDrawListView;

class DepthRenderer {
public:
  explicit DepthRenderer();

  void render(const SceneDrawListView& draw_list_view,
              const CameraContext&     camera_ctx,
              FrameBuffer&             frame_buffer,
              RenderStatus&            render_status);

private:
  ShaderVariant depth_shader_ = DepthShader{};
  RenderState opaque_render_state_;
};

} // namespace Medium
