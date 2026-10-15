///
/// @file   SceneRenderer.cpp
/// @author Garfield949
/// @date   2026/5/27 20:40
/// @brief
///
#include "SceneRenderer.h"

#include "Pipeline/Shadow/CSM.h"
#include "Pipeline/DrawQueue.h"
#include "Pipeline/RenderStatus.h"
#include "Pipeline/DrawCore.h"
#include "Pipeline/FrameBuffer.h"
#include "Resources/CubeMap.h"
#include "Scene/Camera.h"
#include "Scene/MeshNode.h"
#include "Scene/Light/DirectionalLight.h"

namespace Medium {

bool SceneRenderer::init(const Camera* camera, const DirectionalLight* dir_light)
{
  bind(camera, dir_light);

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

  return true;
}

void SceneRenderer::bind(const Camera *camera, const DirectionalLight* dir_light) noexcept {
  main_camera_ = camera;
  dir_light_   = dir_light;
}

void SceneRenderer::render(std::span<const MeshNode* const> scene_mesh_nodes,
                           const CSMContext* csm_ctx,
                           CubeMap*          skybox_cubemap,
                           FrameBuffer&      frame_buffer,
                           RenderStatus&     render_status)
{
  frame_buffer.clearColor();
  frame_buffer.clearDepth();
  
  opaque_queue_.clear();
  transp_queue_.clear();

  CameraContext cam_ctx {
    .cam_pos  = main_camera_->transform.worldPosition(),
    .mat_view = main_camera_->viewMatrix(),
    .mat_vp   = main_camera_->viewProjMatrix(),
    .frustum  = main_camera_->frustum()
  };

  LightingContext lighting_ctx {
    .light_dir   = dir_light_->dir(),
    .light_color = dir_light_->color,
    .csm_ctx     = csm_ctx
  };

  ShaderBindContext shader_ctx{
    .camera_ctx   = &cam_ctx,
    .lighting_ctx = &lighting_ctx
  };

  // ---------- Opaque Pass
  RenderPassContext opaque_pass_ctx{
    .render_state  = opaque_render_state_,
    .viewport      = {0, 0, frame_buffer.widthF(), frame_buffer.heightF()},
    .buffer        = frame_buffer,
    .render_status = &render_status
  };

  opaque_queue_.build(scene_mesh_nodes, cam_ctx);
  opaque_queue_.sort();
  Pipeline::executeDrawCommands(opaque_queue_.drawCommands(), shader_ctx, opaque_pass_ctx, blinn_shader_);

  // ---------- Skybox Pass
  if(opaque_render_state_.draw_mode == DrawMode::Shaded)
    drawSkybox(frame_buffer, skybox_cubemap, render_status);

  // ---------- Transparent Pass
  RenderPassContext transparent_pass_ctx{
    .render_state  = transp_render_state_,
    .viewport      = {0, 0, frame_buffer.widthF(), frame_buffer.heightF()},
    .buffer        = frame_buffer,
    .render_status = &render_status
  };

  transp_queue_.build(scene_mesh_nodes, cam_ctx);
  transp_queue_.sort();
  Pipeline::executeDrawCommands(transp_queue_.drawCommands(), shader_ctx, transparent_pass_ctx, blinn_shader_);

  // 统计不透明 & 不透明 pass draw calls.
  render_status.statistics.scene_opaque_draw_calls += opaque_queue_.drawCommands().size();
  render_status.statistics.scene_transp_draw_calls += transp_queue_.drawCommands().size();
}

void SceneRenderer::drawSkybox(FrameBuffer&  frame_buffer,
                               CubeMap*      skybox_cubemap,
                               RenderStatus& render_status) const
{
  const i32 w   = frame_buffer.width() , h   = frame_buffer.height();
  const f32 w_f = frame_buffer.widthF(), h_f = frame_buffer.heightF();

  const Mat4f& mat_proj      = main_camera_->projMatrix();
  const Mat3f  mat_world_rot = main_camera_->transform.worldMatrix().topLeftCorner(3,3);
  const f32 zoom_x = mat_proj(0,0);
  const f32 zoom_y = mat_proj(1,1);

  // 从左下角开始遍历，同光栅化核心一样，在像素中心采样。
  for(i32 y = 0; y < h; ++y) {
    f32 y_ndc = 2.0f * (static_cast<f32>(y) + 0.5f) / h_f - 1.0f;
    for(i32 x = 0; x < w; ++x) {
      const size_t frag_idx = frame_buffer.getIndex(x,y);

      // Opaque pass 后，未被遮挡的像素（未写深度的像素）即可为天空盒着色。
      if(frame_buffer.depthBuffer()[frag_idx] != FrameBuffer::kDepthClearVal)
        continue;

      f32 x_ndc = 2.0f * (static_cast<f32>(x) + 0.5f) / w_f - 1.0f;

      // 只关心方向，设 z = 1.
      const Vec3f dir_view{x_ndc / zoom_x, y_ndc / zoom_y, 1.0f};
      // 将视线向量转换到世界空间。
      const Vec3f dir_world = mat_world_rot * dir_view;

      frame_buffer.writeHDRColor(frag_idx, skybox_cubemap->sample(dir_world));

      render_status.statistics.written_pixels++;
    }
  }
}

} // namespace Medium
