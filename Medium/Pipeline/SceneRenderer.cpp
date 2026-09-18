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
                           FrameBuffer&  frame_buffer,
                           RenderStatus& render_status)
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

} // namespace Medium
