///
/// @file   RenderManager.cpp
/// @author Garfield949
/// @date   2026/6/21 15:22
/// @brief  
///
#include "RenderManager.h"

#include "Pipeline/RenderContext.h"
#include "Scene/SceneManager.h"
#include "Scene/Light/DirectionalLight.h"
#include "Pipeline/PostProcess/ToneMapping.h"

namespace Medium {

bool RenderManager::init(Scene* scene, const Vec2i& buffer_extent_physical) 
{
  if(!frame_buffer_.resize(buffer_extent_physical.x(),
                           buffer_extent_physical.y())) return false;
  
  if(!shadow_manager_.init(scene->mainCamera(),
                           scene->directionalLight(),
                           scene->shadowConfig().max_shadow_dist,
                           scene->shadowConfig().csm_cascade_cnt))
    return false;
  
  if(!scene_renderer_.init(scene->mainCamera(),
                           scene->directionalLight()))
    return false;

  active_scene_ = scene;

  return true;
}

void RenderManager::bindNewScene(Scene* new_scene) {
  active_scene_ = new_scene;

  shadow_manager_.bind(new_scene->mainCamera(),
                       new_scene->directionalLight(),
                       new_scene->shadowConfig().max_shadow_dist,
                       new_scene->shadowConfig().csm_cascade_cnt);

  scene_renderer_.bind(new_scene->mainCamera(), new_scene->directionalLight());
}

void RenderManager::render() {
  render_status_.reset();
  // 临时输出场景的 XZ 轴尺寸。
  // auto aabb = active_scene_->worldAABB();
  // std::cout << "Extent X: " << aabb.max.x() - aabb.min.x() << std::endl;
  // std::cout << "Extent Z: " << aabb.max.z() - aabb.min.z() << std::endl;
  // ---------- Shadow Pass ----------
  const bool shadow_pass_enabled = shadowingEnabled();
  
  if(shadow_pass_enabled && !shadow_manager_.CSMFrozen())
    shadow_manager_.render(active_scene_->meshNodes(), 
                           active_scene_->worldAABB(), 
                           render_status_);

  // ---------- Main Pass ----------
  scene_renderer_.render(active_scene_->meshNodes(),
                         shadow_pass_enabled ? &shadow_manager_.CSMCtx() : nullptr,
                         frame_buffer_,
                         render_status_);

  if(draw_aabbs_)
    debug_renderer_.drawAABBs(active_scene_->mainCamera()->viewProjMatrix(),
                              scene_renderer_.drawListView(),
                              frame_buffer_, 
                              render_status_);

  // ---------- Post Process ----------
  PostProcess::toneMapping(frame_buffer_);
}

bool RenderManager::onResize(const Vec2i& buffer_extent_physical) {
  if(!frame_buffer_.resize(buffer_extent_physical.x(), buffer_extent_physical.y())) {
    Log::printLogPrefix("RenderManager", "onResize", true);
    fmt::print("Failed\n", buffer_extent_physical.x(), buffer_extent_physical.y());
    return false;
  }
  return true;
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