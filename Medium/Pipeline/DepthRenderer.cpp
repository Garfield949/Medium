///
/// @file   DepthRenderer.cpp
/// @author Garfield949
/// @date   2026/10/31 10:15
/// @brief  
///
#include "DepthRenderer.h"

#include "DrawCore.h"
#include "FrameBuffer.h"
#include "RenderContext.h"
#include "SceneDrawListView.h"

namespace Medium {

DepthRenderer::DepthRenderer() {
  opaque_render_state_ = {
    .enable_depth_test  = true,
    .enable_depth_write = true,
    .enable_color_write = false,
    .draw_mode          = DrawMode::Shaded,
    .cull_mode          = CullMode::Back,
    .depth_cmp          = CompareOP::Greater,
    .blend_mode         = BlendMode::Replace
  };
}

void DepthRenderer::render(const SceneDrawListView& draw_list_view,
                           const CameraContext&     camera_ctx,
                           FrameBuffer&             frame_buffer,
                           RenderStatus&            render_status)
{
  ShaderBindContext shader_ctx{
    .camera_ctx   = &camera_ctx
  };

  RenderPassContext opaque_pass_ctx{
    .render_state  = opaque_render_state_,
    .viewport      = {0, 0, frame_buffer.widthF(), frame_buffer.heightF()},
    .buffer        = frame_buffer,
    .render_status = &render_status
  };
  Pipeline::executeDrawCommands(draw_list_view.opaque, shader_ctx, opaque_pass_ctx, depth_shader_);

  render_status.statistics.depth_opaque_draw_calls += draw_list_view.opaque.size();
}

} // namespace Medium