///
/// @file   ShadowManager.cpp
/// @author Garfield949
/// @date   2026/5/9 17:04
/// @brief  
///
#include "ShadowManager.h"

#include "Scene/Light/DirectionalLight.h"
#include "Pipeline/Shadow/CSM.h"
#include "Pipeline/Shadow/DirectionalViewFit.h"
#include "Pipeline/RenderStat.h"
#include "Pipeline/Pipeline.h"
#include "Pipeline/RenderContext.h"
#include "Pipeline/RenderQueue.h"

namespace Medium {

bool ShadowManager::init(Camera* camera,
                         DirectionalLight* dir_light,
                         u32 csm_tile_res,
                         f32 max_shadow_dist,
                         u32 cascade_count)
{
  // shadow_map_res 是单级 tile 的分辨率。
  if(!shadow_map_atlas_.resize(csm_tile_res * 2)) return false;

  csm_ctx_.shadow_map_atlas = &shadow_map_atlas_;

  csm_tile_res_ = csm_tile_res;
  
  attach(camera, dir_light, max_shadow_dist, cascade_count);

  return true;
}

void ShadowManager::attach(Camera* camera,
            DirectionalLight* dir_light,
            f32 max_shadow_dist,
            u32 cascade_count) noexcept {
  camera_ = camera;
  dir_light_ = dir_light;
  updateCSMSlices(max_shadow_dist, cascade_count);
}

void ShadowManager::updateCSMSlices(f32 max_shadow_dist, u32 cascade_count) noexcept {
  if(max_shadow_dist == max_shadow_dist_ && cascade_count == cascade_cnt_) 
    return;
  
  cascade_cnt_ = cascade_count;
  max_shadow_dist_ = max_shadow_dist;
  csm_ctx_.cascade_cnt = cascade_count;
  
  csm_slices_ = calcCSMSlices(camera_->near(), max_shadow_dist, cascade_count);
  // 计算各 tile 视区
  for(u32 i = 0; i < cascade_cnt_; ++i)
    cascade_viewports_[i] = calcCSMAtlasViewport(i, csm_tile_res_);
}

void ShadowManager::render(std::span<const MeshNode* const> scene_mesh_nodes,
                           const AABB3D& bbox_scene_ws,
                           RenderStat& stat)
{
  shadow_map_atlas_.clearDepth();

  // 计算 CSMFrameCtx 各级联数据。
  for(i32 i = 0; i < static_cast<i32>(cascade_cnt_); ++i) {
    f32 fit_near = csm_slices_[i];
    f32 fit_far  = csm_slices_[i + 1];

    // 从 CSM1 开始将拟合的 near 边界往前一级联扩张一些。
    if(i > 0) fit_near = csm_slices_[i] - csm_ctx_.blendBandWidth(i - 1);

    DirectionalViewFitParams fit_params{
      .camera        = camera_,
      .light_dir     = dir_light_->dir,
      .shadow_res    = csm_tile_res_,
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
      .camera_ctx = &cam_ctx
    };

    RenderPassCtx opaque_pass_ctx{
      .render_state = {
        .enable_depth_test  = true,
        .enable_depth_write = true,
        .enable_color_write = false,
        .cull_mode  = CullMode::CullBack,
        .blend_mode = BlendMode::Replace
      },
      .viewport = cascade_viewports_[i],
      .buffer   = shadow_map_atlas_
    };

    // 透明物体会被剔除掉，实现光绕过车玻璃在车内形成影子。
    RenderQueue queue{RenderQueue::Type::OpaqueQueue};
    queue.build(scene_mesh_nodes, cam_ctx);
    queue.sort();

    Pipeline::executeCommands(queue.commands(), shader_ctx, opaque_pass_ctx, depth_shader_);

    // 收集不透明 pass draw calls.
    stat.statistics.shadow_opaque_draw_calls += queue.commands().size();

    // 收集 CSM 信息.
    stat.csm.max_shadow_distance = max_shadow_dist_;
    stat.csm.cascade_count       = cascade_cnt_;
    stat.csm.csm_slices          = csm_slices_;
  }
}

} // namespace Medium