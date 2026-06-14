///
/// @file   ForwardRenderer.cpp
/// @author Garfield949
/// @date   2026/5/27 20:40
/// @brief  
///
#include "ForwardRenderer.h"

#include "Pipeline/Shadow/CSM.h"
#include "Pipeline/RenderContext.h"
#include "Pipeline/RenderQueue.h"
#include "Pipeline/Pipeline.h"
#include "Scene/Camera.h"

namespace Medium {

bool ForwardRenderer::init(u32 window_width, 
                           u32 window_height, 
                           Camera* camera, 
                           Vec3f light_dir) 
{
  camera_ = camera;
  light_dir_ = std::move(light_dir);

  if(!frame_buffer_.resize(window_width, window_height)) return false;
  
  return true;
}

void ForwardRenderer::render(std::span<const MeshNode* const> scene_mesh_nodes,
                             const CSMSamplingCtx* csm_data)
{
  frame_buffer_.clearColor();
  frame_buffer_.clearDepth();

  CameraCtx cam_ctx {
    .cam_pos = camera_->transform.worldPosition(),
    .view    = camera_->viewMatrix(),
    .vp      = camera_->viewProjMatrix(),
    .frustum = camera_->frustum()
  };

  LightingCtx lighting_ctx {
    .light_dir = light_dir_,
    .csm_data  = csm_data
  };

  ShaderBindCtx shader_ctx{
    .camera_ctx   = &cam_ctx,
    .lighting_ctx = &lighting_ctx,
  };

  RenderPassCtx opaque_pass_ctx{
    .render_state = {
      .blend_mode  = BlendMode::Replace, 
      .color_write = true,
      .depth_write = true,
      .depth_test  = true
    },
    .viewport = {0, 0, frame_buffer_.widthF(), frame_buffer_.heightF()},
    .buffer   = frame_buffer_
  };
  
  RenderQueue opaque_queue{RenderQueue::Type::OpaqueQueue};
  opaque_queue.build(scene_mesh_nodes, cam_ctx);
  opaque_queue.sort();
  Pipeline::executeCommands(opaque_queue.commands(), shader_ctx, opaque_pass_ctx, blinn_shader_);

  RenderPassCtx transparent_pass_ctx{
    .render_state = {
      .blend_mode  = BlendMode::PremultipliedOver, 
      .color_write = true,
      .depth_write = false, // 透明队列关闭深度写入，否则在前面的透明物体会占用深度缓冲，导致后面的透明物体直接被丢弃无法参与正常混合。
      .depth_test  = true
    },
    .viewport = {0, 0, frame_buffer_.widthF(), frame_buffer_.heightF()},
    .buffer   = frame_buffer_
  };

  RenderQueue transparent_queue{RenderQueue::Type::TransparentQueue};
  transparent_queue.build(scene_mesh_nodes, cam_ctx);
  transparent_queue.sort();
  Pipeline::executeCommands(transparent_queue.commands(), shader_ctx, transparent_pass_ctx, blinn_shader_);

  // ---------- Post Process ----------
  frame_buffer_.toneMap();
}

} // Medium