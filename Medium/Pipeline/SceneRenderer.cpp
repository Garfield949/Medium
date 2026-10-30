///
/// @file   SceneRenderer.cpp
/// @author Garfield949
/// @date   2026/5/27 20:40
/// @brief
///
#include "SceneRenderer.h"

#include "Pipeline/Shadow/CSM.h"
#include "Pipeline/RenderStatus.h"
#include "Pipeline/DrawCore.h"
#include "Pipeline/FrameBuffer.h"
#include "Pipeline/SceneDrawListView.h"

#include "Resources/SkyBox.h"
#include "Scene/Light/DirectionalLight.h"

namespace Medium {

namespace {

void drawSkybox(const CameraContext& camera_ctx,
                const CubeMap&       skybox_cubemap,
                FrameBuffer&         frame_buffer,
                RenderStatus&        render_status)
{
  const i32 w   = frame_buffer.width() , h   = frame_buffer.height();
  const f32 w_f = frame_buffer.widthF(), h_f = frame_buffer.heightF();

  const Mat4f& mat_proj      = camera_ctx.mat_proj;
  const Mat3f  mat_world_rot = camera_ctx.mat_world.topLeftCorner(3,3);
  const f32 zoom_x = mat_proj(0,0);
  const f32 zoom_y = mat_proj(1,1);

  // 从左下角开始遍历，同光栅化核心一样，在像素中心采样。
  for(i32 y = 0; y < h; ++y) {
    f32 y_ndc = 2.0f * (static_cast<f32>(y) + 0.5f) / h_f - 1.0f;
    for(i32 x = 0; x < w; ++x) {
      const size_t frag_idx = frame_buffer.getIndex(x,y);

      // main opaque pass 后，未被遮挡的像素（未写深度的像素）即可为天空盒着色。
      if(frame_buffer.depthBuffer()[frag_idx] != FrameBuffer::kDepthClearVal)
        continue;

      f32 x_ndc = 2.0f * (static_cast<f32>(x) + 0.5f) / w_f - 1.0f;

      // 只关心方向，设 z = 1.
      const Vec3f dir_view{x_ndc / zoom_x, y_ndc / zoom_y, 1.0f};

      // 将视线向量转换到世界空间。
      const Vec3f dir_world = mat_world_rot * dir_view;

      frame_buffer.writeHDRColor(frag_idx, skybox_cubemap.sample(dir_world));

      render_status.statistics.written_pixels++;
    }
  }
}

}


SceneRenderer::SceneRenderer() {
  opaque_render_state_ = {
    .enable_depth_test  = true,
    .enable_depth_write = true,
    .enable_color_write = true,
    .draw_mode          = DrawMode::Shaded,
    .cull_mode          = CullMode::Back,
    .blend_mode         = BlendMode::Replace,
    .color_write_target = ColorWriteTarget::HDR
  };
  
  transp_render_state_ = {
    .enable_depth_test  = true,
    .enable_depth_write = false, // 透明队列关闭深度写入，否则在前面的透明物体会占用深度缓冲，导致后面的透明物体直接被丢弃无法参与正常混合。
    .enable_color_write = true,
    .draw_mode          = DrawMode::Shaded,
    .cull_mode          = CullMode::Back,
    .blend_mode         = BlendMode::PremultipliedOver,
    .color_write_target = ColorWriteTarget::HDR
  };
}

void SceneRenderer::render(const SceneDrawListView& draw_list_view,
                           const CameraContext&     camera_ctx,
                           const DirectionalLight*  dir_light,
                           const CSMContext*        csm_ctx,
                           const SkyBox*            skybox,
                           FrameBuffer&             frame_buffer,
                           RenderStatus&            render_status)
{
  frame_buffer.clearColor();
  frame_buffer.clearDepth();

  LightingContext lighting_ctx {
    .light_dir   = dir_light->dir(),
    .light_color = dir_light->color,
    .csm_ctx     = csm_ctx,
    .sh9         = &skybox->sh9
  };

  ShaderBindContext shader_ctx{
    .camera_ctx   = &camera_ctx,
    .lighting_ctx = &lighting_ctx
  };

  // ---------- Opaque Pass
  RenderPassContext opaque_pass_ctx{
    .render_state  = opaque_render_state_,
    .viewport      = {0, 0, frame_buffer.widthF(), frame_buffer.heightF()},
    .buffer        = frame_buffer,
    .render_status = &render_status
  };
  Pipeline::executeDrawCommands(draw_list_view.opaque, shader_ctx, opaque_pass_ctx, blinn_shader_);

  // ---------- Skybox Pass
  if(opaque_render_state_.draw_mode == DrawMode::Shaded)
    drawSkybox(camera_ctx, skybox->cubemap, frame_buffer, render_status);

  // ---------- Transparent Pass
  RenderPassContext transparent_pass_ctx{
    .render_state  = transp_render_state_,
    .viewport      = {0, 0, frame_buffer.widthF(), frame_buffer.heightF()},
    .buffer        = frame_buffer,
    .render_status = &render_status
  };
  Pipeline::executeDrawCommands(draw_list_view.transparent, shader_ctx, transparent_pass_ctx, blinn_shader_);

  render_status.statistics.scene_opaque_draw_calls += draw_list_view.opaque.size();
  render_status.statistics.scene_transp_draw_calls += draw_list_view.transparent.size();
}

} // namespace Medium
