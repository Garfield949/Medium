///
/// @file   ShadowManager.cpp
/// @author Garfield949
/// @date   2026/5/9 17:04
/// @brief  
///
#include "ShadowManager.h"

#include "Pipeline/Pipeline.h"
#include "Pipeline/RenderContext.h"
#include "Pipeline/RenderQueue.h"

namespace Medium {

void ShadowManager::render(std::span<const MeshNode* const> mesh_nodes,
                           const AABB3D& bbox_scene_ws) {
  shadow_map_->clearDepthBuf();

  // 构造平行光视椎拟合参数
  // TODO: 取消掉 DirectionalViewFitDesc，改为传参
  DirectionalViewFitDesc fit_desc{
    .camera     = main_camera_,
    .light_dir  = main_light_->dir,
    .shadow_res = shadow_map_->width(),
    .slice_far  = 5.0f,
    .bbox_scene_ws = bbox_scene_ws
  };

  // 获取平行光视椎拟合结果并重建平行光空间
  auto dir_light_view_fit = calcDirectionalViewFit(fit_desc, DirectionalFitMode::Tight);
  main_light_->applyViewFit(dir_light_view_fit);

  FrameCtx frame_ctx {
    .main_light = main_light_,
    .shadow_map = shadow_map_.get()
  };

  shadow_texel_size_ws_ = dir_light_view_fit.shadow_texel_size_ws;

  ViewCtx view_ctx_shadow { main_light_->attachedCamera() };

  PassCtx pass_ctx_shadow {
    .frame_ctx = &frame_ctx,
    .view_ctx  = &view_ctx_shadow,
    .buffer    = shadow_map_.get(),
    .shader    = &depth_shader_,
  };
  
  RenderQueue shadow_caster_queue{RenderQueue::Type::Shadow};
  shadow_caster_queue.build(mesh_nodes, view_ctx_shadow);
  shadow_caster_queue.sort();

  Pipeline::executeCommands(shadow_caster_queue.commands(), pass_ctx_shadow);
}

} // namespace Medium