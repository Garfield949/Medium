///
/// @file App.cpp
/// @brief
///
#include "App.h"

#include <fmt/base.h>
#include <SDL3/SDL_events.h>

#include "App/Config/Config.h"
#include "Base/Log.h"
#include "Scene/Scene.h"
#include "Scene/Light/DirectionalLight.h"
#include "Pipeline/RenderStat.h"

namespace Medium {

namespace fs = std::filesystem;

void App::loadConfig() {
  app_config_ = AppCfg::LoadFromToml(fs::path(PROJECT_ROOT_DIR) / Config::kConfigDirName);
}

bool App::loadScene() {
  const WindowCfg& cfg_window = app_config_.window;

  auto lua_path = fs::path(PROJECT_ROOT_DIR) / "scenes"
    / "sponza.lua";

  active_scene_ = Scene::LoadFromLua(lua_path, cfg_window.aspectRatio());

  if(!active_scene_) {
    printLogPrefix("App", "loadScene", true);
    fmt::print("Initialize Failed\n");
    return false;
  }

  return true;
}

void App::setupTools() noexcept {
  controller_.bind(active_scene_->camera(), &input_);
  profiler_.setReportInterval(1.0f);
}

bool App::init() {
  loadConfig();

  if(!window_manager_.init(app_config_.window)) return false;

  if(!loadScene()) return false;

  setupTools();

  if(!shadow_manager_.init(active_scene_->camera(),
                           active_scene_->dirLight(),
                           app_config_.render.shadow.shadow_map_res,
                           active_scene_->shadowCfg().max_shadow_dist,
                           active_scene_->shadowCfg().csm_cascade_cnt)) return false;

  auto [width_physical, height_physical] = window_manager_.physicalExtent();
  
  if(!forward_renderer_.init(width_physical,
                             height_physical,
                             active_scene_->camera(),
                             active_scene_->dirLight()->dir)) return false;

  return true;
}

void App::loop() {
  while(true) {
    // 使用一个循环不断从事件队列中取出并消费下一个事件，非阻塞。
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
      switch(e.type) {
      case SDL_EVENT_QUIT:
        return;
      case SDL_EVENT_KEY_DOWN:
        if(e.key.key == SDLK_ESCAPE) return;
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        if(!onWindowResize()) return;
        break;
      default: break;
      }
    }

    onUpdate(frame_timer_.tick());

    onRender();
  }
}

void App::onUpdate(const f32 dt) {
  input_.update();
  profiler_.update(dt);
  controller_.update(dt);
}

void App::onRender() {
  RenderStat render_stat{};

  // ---------- Shadow Pass ----------
  shadow_manager_.render(active_scene_->meshNodes(),
                         active_scene_->worldAABB());

  // 临时输出场景的 XZ 轴尺寸。
  // auto aabb = active_scene_->worldAABB();
  // std::cout << "Extent X: " << aabb.max.x() - aabb.min.x() << std::endl;
  // std::cout << "Extent Z: " << aabb.max.z() - aabb.min.z() << std::endl;

  // ---------- Main Pass ----------
  forward_renderer_.render(active_scene_->meshNodes(),
                           shadow_manager_.CSMData());

  const FrameBuffer* fb = forward_renderer_.frameBuffer();

  window_manager_.commitBuffer(fb->presentColorBuffer(), fb->width(), fb->height());
}

[[nodiscard]]
bool App::onWindowResize() {
  auto [w_physical, h_physical] = window_manager_.physicalExtent();

  FrameBuffer* fb = forward_renderer_.frameBuffer();
  if(static_cast<u32>(w_physical) == fb->width() && static_cast<u32>(h_physical) == fb->height())
    return true;

  // 测试过，窗口手动拉伸的最小尺寸是有下限的。出现 0 只能说明出现异常。
  if(w_physical <= 0 || h_physical <= 0) {
    printLogPrefix("App", "onWindowResize", true);
    fmt::print("Invalid window size: [{} x {}]\n", w_physical, h_physical);
    return false;
  }

  if(!forward_renderer_.frameBuffer()->resize(w_physical, h_physical)) {
    printLogPrefix("App", "onWindowResize", true);
    fmt::print("Failed\n", w_physical, h_physical);
    return false;
  }

  // 注：更改 aspect ratio 后，相机视椎形状会变，进而使阴影拟合产生变化
  active_scene_->camera()->setAspectRatio(static_cast<f32>(w_physical) / static_cast<f32>(h_physical));

  window_manager_.onResize();

  return true;
}

void App::quit() {
  app_config_.saveToTomlFile();
}

} // namespace Medium
