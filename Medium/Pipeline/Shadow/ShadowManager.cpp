///
/// @file   ShadowManager.cpp
/// @author Garfield949
/// @date   2026/5/9 17:04
/// @brief  
///
#include "ShadowManager.h"

#include "Base/Log.h"
#include "Base/Color.h"
#include "Scene/Camera.h"
#include "Scene/Light/DirectionalLight.h"
#include "Pipeline/Shadow/CSM.h"
#include "Pipeline/Shadow/DirectionalViewFit.h"
#include "Pipeline/RenderStatus.h"
#include "Pipeline/DrawCore.h"
#include "Pipeline/RenderContext.h"
#include "Pipeline/DrawQueue.h"

namespace Medium {

bool ShadowManager::init(const Camera* main_camera,
                         DirectionalLight* dir_light,
                         f32 max_shadow_dist,
                         i32 cascade_count) 
{
  setCascadeTileRes(kDefaultCascadeTileRes);

  csm_ctx_.csm_atlas = &csm_atlas_;
  
  bind(main_camera, dir_light, max_shadow_dist, cascade_count);

  opaque_render_state_ = {
    .enable_depth_test  = true,
    .enable_depth_write = true,
    .enable_color_write = false,
    .draw_mode          = DrawMode::Shaded,
    .cull_mode          = CullMode::Back,
    .blend_mode         = BlendMode::Replace
  };

  return true;
}

void ShadowManager::render(std::span<const MeshNode* const> scene_mesh_nodes,
                           const AABB3D& bbox_scene_ws,
                           RenderStatus& render_status)
{
  auto update_policy = makeUpdatePolicy();
  
  // 若不需更新 Cascade 数据，那么更没必要更新 Atlas，也没必要更新阴影查询坐标系快照，直接返回。
  // 这里 early return 后，mat_view_snapshot 就自然是旧的已冻结快照。
  if(!update_policy.update_cascade_data) return;

  // 获取当前帧的阴影查询坐标系快照，用作冻结 CSM 数据时的 view 矩阵。
  csm_ctx_.mat_view_snapshot = camera_->viewMatrix();

  // 若不需更新 Atlas 则不清空其缓冲。
  if(update_policy.update_atlas) 
    csm_atlas_.clearDepth();

  for(i32 i = 0; i < cascade_cnt_; ++i) {
    f32 fit_near = cascade_slices_[i];
    f32 fit_far  = cascade_slices_[i + 1];

    // 从 CSM1 开始将拟合的 near 边界往前一级联扩张一些。
    if(i > 0) fit_near = cascade_slices_[i] - csm_ctx_.blendBandWidth(i - 1);

    DirectionalViewFitParams fit_params{
      .camera        = camera_,
      .light_dir     = dir_light_->dir(),
      .shadow_res    = cascade_tile_res_,
      .slice_near    = fit_near,
      .slice_far     = fit_far,
      .scene_bbox_ws = bbox_scene_ws
    };

    DirectionalViewFit fit = calcDirectionalViewFit(fit_params);
    dir_light_->applyViewFit(fit);

    csm_ctx_.cascades[i] = {
      .slice_near     = cascade_slices_[i],
      .slice_far      = cascade_slices_[i + 1],
      .mat_vp_light   = dir_light_->viewProjMatrix(),
      .viewport_atlas = cascade_viewports_[i],
      .texel_size_ws  = fit.shadow_texel_size_ws
    };

    // 若不需要更新 Atlas，那么就不执行 Shadow Pass.
    if(!update_policy.update_atlas) continue;

    const Camera* light_cam = dir_light_->attachedCamera();
    CameraContext light_cam_ctx{
      .cam_pos  = light_cam->transform.worldPosition(),
      .mat_view = light_cam->viewMatrix(),
      .mat_vp   = light_cam->viewProjMatrix(),
      .frustum  = light_cam->frustum()
    };

    ShaderBindContext shader_ctx{
      .camera_ctx = &light_cam_ctx
    };

    RenderPassContext opaque_pass_ctx{
      .render_state = opaque_render_state_,
      .viewport     = cascade_viewports_[i],
      .buffer       = csm_atlas_
    };

    // 透明物体会被剔除掉，实现光绕过车玻璃在车内形成影子。
    opaque_queue_.clear();
    opaque_queue_.build(scene_mesh_nodes, light_cam_ctx);
    opaque_queue_.sort();

    Pipeline::executeDrawCommands(opaque_queue_.drawCommands(), shader_ctx, opaque_pass_ctx, depth_shader_);

    // 收集不透明 pass draw calls.
    render_status.statistics.shadow_opaque_draw_calls += opaque_queue_.drawCommands().size();
  }
  
  if(csm_freeze_state_ == CSMFreezeState::Request) csm_freeze_state_ = CSMFreezeState::Frozen;
}

void ShadowManager::bind(const Camera* main_camera,
                         DirectionalLight* dir_light,
                         f32 max_shadow_dist,
                         i32 cascade_count) noexcept 
{
  camera_ = main_camera;
  dir_light_ = dir_light;
  setCascadeCount(cascade_count);
  setMaxShadowDistance(max_shadow_dist);
}

void ShadowManager::setCascadeCount(i32 count) noexcept {
  count = std::clamp(count, 1, kMaxCSMCascades);
  if(cascade_cnt_ == count) return;
  cascade_cnt_ = count;
  csm_ctx_.cascade_cnt = cascade_cnt_;
  updateCascadeSlices();
}

void ShadowManager::setMaxShadowDistance(f32 dist) noexcept {
  if(max_shadow_dist_ == dist) return;
  max_shadow_dist_ = std::max(Camera::kNearPlane * 2, dist);
  updateCascadeSlices();
}

void ShadowManager::setCascadeTileRes(i32 res) {
  if(cascade_tile_res_ == res) return;

  resize(res);

  cascade_tile_res_ = res;
  updateCascadeViewports();
}

void ShadowManager::resize(i32 tile_res) {
  csm_atlas_.resize(tile_res * 2);
  csm_atlas_debug_.resize(tile_res / 2);
}

void ShadowManager::updateCascadeSlices() noexcept {
  cascade_slices_ = calcCSMSlices(Camera::kNearPlane, max_shadow_dist_, cascade_cnt_);
}

void ShadowManager::updateCascadeViewports() noexcept {
  // 不管 cascade_cnt_ 是多少，总是更新 kMaxCSMCascades 个级联的视区。
  for(i32 i = 0; i < kMaxCSMCascades; ++i)
    cascade_viewports_[i] = getCascadeViewport(i, cascade_tile_res_);
}

void ShadowManager::updateAtlasDebugTexture() noexcept {
  const Image2D<f32>& depth_buf = csm_atlas_.depthBuffer();

  i32 w_dst = csm_atlas_debug_.width();
  i32 h_dst = csm_atlas_debug_.height();

  constexpr i32 kScale = 4;

  for(i32 y = 0; y < h_dst; ++y) {
    for(i32 x = 0; x < w_dst; ++x) {
      // 这里不用线性平均，否则归一化的深度平均下来什么都看不清。
      // 转而选一个 4*4 核里的最大深度值（Reversed-Z 下即离镜头最近的点）代表这一块：近处的值越大，越接近白色。
      f32 z_max = 0.0f;
      for(int j = 0; j < kScale; ++j) {
        for(int i = 0; i < kScale; ++i) {
          const size_t src_idx = csm_atlas_.getIndex(x * kScale + i, y * kScale + j);
          if(const f32 src_depth = depth_buf[src_idx]; z_max < src_depth)
            z_max = src_depth;
        }
      }
      const u8 grey_scale = quantizeToU8(std::clamp(z_max, 0.0f, 1.0f));
      csm_atlas_debug_[x, y] = Color{grey_scale};
    }
  }
}

ShadowManager::CSMUpdatePolicy ShadowManager::makeUpdatePolicy() const noexcept {
  const bool unfrozen = csm_freeze_state_ == CSMFreezeState::UnFrozen;
  const bool request  = csm_freeze_state_ == CSMFreezeState::Request;
  
  return {
    .update_cascade_data = unfrozen || request,
    .update_atlas = request || (unfrozen && !(csm_ctx_.cascade_band_view_enabled && !atlas_preview_requested_))
     // 当状态为 unfrozen，除了「显示级联带」+「不显示 atlas」的情况，其它时候均要更新 atlas.
     // 当状态为 request，那么不管是什么情况都必须进行一次强制更新。
  };
}

} // namespace Medium