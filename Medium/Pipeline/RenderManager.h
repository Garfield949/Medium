///
/// @file   RenderManager.h
/// @author Garfield949
/// @date   2026/6/21 15:22
/// @brief  
///
#pragma once

#include "Pipeline/RenderStat.h"
#include "Pipeline/SceneRenderer.h"
#include "Pipeline/Shadow/ShadowManager.h"

namespace Medium {

class Scene;;
struct RenderCfg;

class RenderManager {
public:
  explicit RenderManager() = default;

  bool init(Scene* scene,
            const RenderCfg& render_config,
            const Vec2i& buffer_extent_physical);

  void bindNewScene(Scene* new_scene);

  void render();

  bool onResize(const Vec2i& buffer_extent_physical);

  FrameBuffer& frameBuffer() noexcept { return scene_renderer_.frameBuffer(); }

  const RenderStat& renderStat() const noexcept { return render_stat_; }

  RenderStat& renderStat() noexcept { return render_stat_; }

  ShadowManager& shadowManager() noexcept { return shadow_manager_; }

private:
  Scene* active_scene_ = nullptr;
  
  ShadowManager shadow_manager_;
  SceneRenderer scene_renderer_;
  
  RenderStat render_stat_;
};

} // namespace Medium
