///
/// @file   ShadowManager.cpp
/// @author Garfield949
/// @date   2026/5/9 17:04
/// @brief  
///
#include "ShadowManager.h"

#include "Scene/Light/DirectionalLight.h"
#include "Pipeline/Shadow/CSM.h"
#include "Pipeline/Pipeline.h"
#include "Pipeline/RenderContext.h"
#include "Pipeline/RenderQueue.h"

namespace Medium {

void ShadowManager::init(Camera* camera, 
                         DirectionalLight* dir_light,
                         u32 shadow_map_tile_res,
                         f32 max_shadow_dist,
                         i32 cascade_count)
{  
  camera_          = camera;
  dir_light_       = dir_light;

  cascade_cnt_         = cascade_count;
  max_shadow_dist_     = max_shadow_dist;
  shadow_map_tile_res_ = shadow_map_tile_res;

  // shadow_map_res 是单张 tile 的分辨率。
  shadow_map_atlas_ = std::make_unique<RenderTarget>(shadow_map_tile_res << 1, RenderTarget::Mode::ShadowMap);

  csm_ctx_.cascade_cnt      = cascade_cnt_;
  csm_ctx_.shadow_map_atlas = shadow_map_atlas_.get();

  // 计算级联切片逻辑范围。
  csm_slices_ = calcCSMSlices(camera_->near(), max_shadow_dist, cascade_count);

  // 计算各 tile 视区
  for(i32 i = 0; i < cascade_cnt_; i++)
    cascade_viewports_[i] = calcCSMAtlasViewport(i, shadow_map_tile_res);
}

void ShadowManager::render(std::span<const MeshNode* const> scene_mesh_nodes,
                           const AABB3D& bbox_scene_ws)
{
  shadow_map_atlas_->clearDepthBuf();

  // 计算 CSMFrameCtx 各级联数据。
  for(i32 i = 0; i < cascade_cnt_; i++) {
    f32 fit_near = csm_slices_[i];
    f32 fit_far  = csm_slices_[i + 1];

    // 从 CSM1 开始将拟合的 near 边界往前一级联扩张一些。
    if(i > 0) fit_near = csm_slices_[i] - csm_ctx_.blendBandWidth(i - 1);

    DirectionalViewFitParams fit_params{
      .camera        = camera_,
      .light_dir     = dir_light_->dir,
      .shadow_res    = shadow_map_tile_res_,
      .slice_near    = fit_near,
      .slice_far     = fit_far,
      .scene_bbox_ws = bbox_scene_ws
    };

    DirectionalViewFit fit = calcDirectionalViewFit(fit_params);
    dir_light_->applyViewFit(fit);

    csm_ctx_.cascades[i] = {
      .slice_near    = csm_slices_[i],
      .slice_far     = csm_slices_[i + 1],
      .light_vp      = dir_light_->viewProjMatrix(),
      .viewport      = cascade_viewports_[i],
      .texel_size_ws = fit.shadow_texel_size_ws
    };

    const Camera* light_cam = dir_light_->attachedCamera();
    CameraCtx cam_ctx{
      .cam_pos = light_cam->transform.worldPosition(),
      .view    = light_cam->viewMatrix(),
      .vp      = light_cam->viewProjMatrix(),
      .frustum = light_cam->frustum()
    };

    ShaderBindCtx shader_ctx{
      .camera_ctx = cam_ctx
    };

    RenderPassCtx pass_ctx{
      .viewport = cascade_viewports_[i],
      .buffer   = *shadow_map_atlas_
    };

    RenderQueue queue{RenderQueue::Type::Shadow};
    queue.build(scene_mesh_nodes, cam_ctx);
    queue.sort();

    Pipeline::executeCommands(queue.commands(), shader_ctx, pass_ctx, depth_shader_);
  }
}

} // namespace Medium