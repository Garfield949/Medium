///
/// @file App.cpp
/// @brief
///
#include "App.h"

#include <MiniFB.h>

#include "App/Config/Config.h"
#include "Pipeline/RenderStat.h"
#include "Scene/Light/DirectionalLight.h"

#include <iostream>

namespace fs = std::filesystem;

void App::loadConfig() {
  app_config_ = AppCfg::loadFromToml(fs::path(PROJECT_ROOT_DIR) / Config::kConfigDirName);
}

bool App::createWindow() {
  const WindowCfg& cfg_window = app_config_.window;

  mfb_window_ = mfb_open_ex(cfg_window.title.c_str(),
                            cfg_window.w,
                            cfg_window.h,
                            MFB_WF_RESIZABLE);
  if(!mfb_window_) {
    fmt::print(stderr, "App::Init Failed::Cannot create a MiniFB Window.\n");
    return false;
  }
  
  mfb_show_cursor(mfb_window_, true);
  return true;
}

bool App::loadScene() {
  const WindowCfg& cfg_window = app_config_.window;
  
  auto lua_path = fs::path(PROJECT_ROOT_DIR) / "scenes"
      / "living_room.lua";
  
  active_scene_ = Scene::loadFromLua(lua_path, cfg_window.aspectRatio());

  if(!active_scene_) {
    fmt::print(stderr, "App - Initialize Failed\n");
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

  if(!createWindow()) return false;
  
  if(!loadScene()) return false;
  
  setupTools();

  shadow_manager_.init(active_scene_->camera(),
                       active_scene_->dirLight(),
                       app_config_.render.shadow.shadow_map_res,
                       active_scene_->shadowCfg().max_shadow_dist,
                       active_scene_->shadowCfg().csm_cascade_cnt);

  const WindowCfg& cfg_window = app_config_.window;
  forward_renderer_.init(cfg_window.w,
                         cfg_window.h,
                         active_scene_->camera(),
                         active_scene_->dirLight()->dir);

  return true;
}

void App::loop() {
  do {
    const f32 dt = frame_timer_.tick();

    onUpdate(dt);
    
    onWindowResize();

    onRender();

  } while(mfb_wait_sync(mfb_window_));
  // mfb_wait_sync 强制等待垂直同步
  // 若句柄因错误被置空，mfb_wait_sync 返回 false
}

void App::onUpdate(const f32 dt) {
  input_.update(mfb_window_);
  profiler_.update(dt);
  controller_.update(dt);
}

void App::onRender() {
  RenderStat render_stat{};

  // ---------- Shadow Pass ----------
  shadow_manager_.render(active_scene_->meshNodes(),
                         active_scene_->worldAABB());

  // 临时输出场景的 xy 长宽
  // auto aabb = active_scene_->worldAABB();
  // std::cout << "Extent X: " << aabb.max.x() - aabb.min.x() << std::endl;
  // std::cout << "Extent Z: " << aabb.max.z() - aabb.min.z() << std::endl;

  // ---------- Main Pass ----------
  forward_renderer_.render(active_scene_->meshNodes(),
                           shadow_manager_.CSMData());

  // render_stat.draw_calls += opaque_queue.commands().size();
  //
  // profiler_.collectStats(render_stat);

  // mfb_update_state 为用于 mfb_update_ex 返回值的枚举:
  // 1. STATE_OK 更新成功；2. STATE_EXIT 用户按了 esc，或程序请求退出；
  // 3. STATE_INVALID_WINDOW 窗口句柄无效；
  // 4. STATE_INTERNAL_ERROR minifb 内部错误；
  RenderTarget* frame_buffer = forward_renderer_.frameBuffer();
  mfb_update_state state = mfb_update_ex(mfb_window_,
                                         frame_buffer->colorBuf(),
                                         frame_buffer->width(),
                                         frame_buffer->height());
  // 更新异常，则主动断开对窗口句柄的持有
  if (state != MFB_STATE_OK) mfb_window_ = nullptr;
}

void App::onWindowResize() {
  u32 w_new = mfb_get_window_width(mfb_window_);
  u32 h_new = mfb_get_window_height(mfb_window_);
  
  if(w_new == 0 || h_new == 0) return;

  // 注：更改 aspect ratio 后，相机视椎形状会变，进而使阴影拟合产生变化
  active_scene_->camera()->setAspectRatio(
    static_cast<f32>(w_new) / static_cast<f32>(h_new));
  
  forward_renderer_.frameBuffer()->resize(w_new, h_new);
}

void App::quit() {
  app_config_.saveToTomlFile();
}
