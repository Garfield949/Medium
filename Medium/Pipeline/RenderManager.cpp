///
/// @file   RenderManager.cpp
/// @author Garfield949
/// @date   2026/6/21 15:22
/// @brief  
///
#include "RenderManager.h"

#include "Scene/SceneManager.h"
#include "Scene/Light/DirectionalLight.h"
#include "Pipeline/RenderContext.h"
#include "Pipeline/DebugRenderer.h"
#include "Pipeline/PostProcess/ToneMapping.h"

namespace Medium {

void RenderManager::init(Scene* scene, const Vec2i& buffer_extent_physical) 
{
  shadow_manager_.init(scene->shadowConfig().max_shadow_dist,
                       scene->shadowConfig().csm_cascade_cnt);

  frame_buffer_.resize(buffer_extent_physical.x(), buffer_extent_physical.y());

  active_scene_ = scene;
}

void RenderManager::bindNewScene(Scene* new_scene) {
  shadow_manager_.setMaxShadowDistance(new_scene->shadowConfig().max_shadow_dist);
  shadow_manager_.setCascadeCount(new_scene->shadowConfig().csm_cascade_cnt);
  
  active_scene_ = new_scene;
}

void RenderManager::updateCameraContext() noexcept {
  const auto main_cam = active_scene_->mainCamera();
  const auto view = main_cam->viewMatrix();
  const auto proj = main_cam->projMatrix();
  main_cam_ctx_ = {
    .cam_pos   = main_cam->transform.worldPosition(),
    .mat_world = main_cam->transform.worldMatrix(),
    .mat_view  = view,
    .mat_proj  = proj,
    .mat_vp    = proj * view,
    .frustum   = main_cam->frustum()
  };
}

void RenderManager::updateDrawQueues() {
  main_opaque_queue_.clear();
  main_opaque_queue_.build(active_scene_->meshNodes(), main_cam_ctx_);
  main_opaque_queue_.sort();

  main_transp_queue_.clear();
  main_transp_queue_.build(active_scene_->meshNodes(), main_cam_ctx_);
  main_transp_queue_.sort();
}

void RenderManager::render() {
  render_status_.reset();
  updateCameraContext();
  updateDrawQueues();

  // ---------- Shadow Pass ----------
  const bool shadow_pass_enabled = shadowingEnabled();
  
  if(shadow_pass_enabled && !shadow_manager_.CSMFrozen())
    shadow_manager_.render(active_scene_->meshNodes(),
                           active_scene_->mainCamera(),
                           active_scene_->directionalLight(),
                           active_scene_->worldAABB(), 
                           render_status_);

  // ---------- Main Pass ----------
  scene_renderer_.render(sceneDrawListView(),
                         main_cam_ctx_,
                         active_scene_->directionalLight(),
                         shadow_pass_enabled ? &shadow_manager_.CSMCtx() : nullptr,
                         active_scene_->skybox(),
                         frame_buffer_,
                         render_status_);

  if(draw_aabbs_)
    DebugRenderer::drawAABBs(active_scene_->mainCamera()->viewProjMatrix(),
                             sceneDrawListView(),
                             frame_buffer_, 
                             render_status_);

  // ---------- Post Process ----------
  PostProcess::toneMapping(frame_buffer_);
}

void RenderManager::onResize(const Vec2i& buffer_extent_physical) {
  frame_buffer_.resize(buffer_extent_physical.x(), buffer_extent_physical.y());
}

void RenderManager::setDrawMode(DrawMode mode) noexcept {
  draw_mode_ = mode;
  scene_renderer_.setDrawMode(mode);
}

bool RenderManager::shadowingEnabled() const noexcept {
  return draw_mode_ == DrawMode::Shaded &&
         active_scene_->directionalLight()->cast_shadows;
}

} // namespace Medium