///
/// @file   RenderManager.cpp
/// @author Garfield949
/// @date   2026/6/21 15:22
/// @brief  
///
#include "RenderManager.h"

#include "App/Config/Config.h"
#include "Scene/Light/DirectionalLight.h"
#include "Scene/SceneManager.h"

namespace Medium {

bool RenderManager::init(Scene* scene,
                         const RenderCfg& render_config, 
                         const Vec2i& buffer_extent_physical) 
{
  active_scene_ = scene;
  
  if(!shadow_manager_.init(scene->camera(),
                           scene->dirLight(),
                           render_config.shadow_map_res,
                           scene->shadowCfg().max_shadow_dist,
                           scene->shadowCfg().csm_cascade_cnt))
    return false;
  
  if(!scene_renderer_.init(buffer_extent_physical,
                           scene->camera(),
                           scene->dirLight()->dir))
    return false;

  return true;
}

void RenderManager::bindNewScene(Scene* new_scene) {
  active_scene_ = new_scene;

  shadow_manager_.attach(new_scene->camera(),
                       new_scene->dirLight(),
                       new_scene->shadowCfg().max_shadow_dist,
                       new_scene->shadowCfg().csm_cascade_cnt);

  scene_renderer_.bind(new_scene->camera(), new_scene->dirLight()->dir);
}

void RenderManager::render() {
  render_stat_.reset();
  // 临时输出场景的 XZ 轴尺寸。
  // auto aabb = active_scene_->worldAABB();
  // std::cout << "Extent X: " << aabb.max.x() - aabb.min.x() << std::endl;
  // std::cout << "Extent Z: " << aabb.max.z() - aabb.min.z() << std::endl;
  // ---------- Shadow Pass ----------
  shadow_manager_.render(active_scene_->meshNodes(),
                         active_scene_->worldAABB(),
                         render_stat_);

  // ---------- Main Pass ----------
  scene_renderer_.render(active_scene_->meshNodes(),
                         shadow_manager_.CSMData(),
                         render_stat_);
}

bool RenderManager::onResize(const Vec2i& buffer_extent_physical) {
  if(!scene_renderer_.frameBuffer().resize(buffer_extent_physical.x(), buffer_extent_physical.y())) {
    Log::printLogPrefix("RenderManager", "onResize", true);
    fmt::print("Failed\n", buffer_extent_physical.x(), buffer_extent_physical.y());
    return false;
  }
  return true;
}

} // namespace Medium