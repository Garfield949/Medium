///
/// @file App.cpp
/// @brief
///
#include <MiniFB.h>

#include "App.h"

#include "App/Config/Config.h"
#include "Scene/Scene.h"
#include "Scene/MeshNode.h"
#include "Pipeline/Pipeline.h"
#include "Pipeline/RenderContext.h"
#include "Pipeline/RenderQueue.h"
#include "Pipeline/Shader/VariantShader.h"

namespace fs = std::filesystem;

App::App() {
  app_config_ = AppCfg::loadFromToml(fs::path(PROJECT_ROOT_DIR) / Config::kConfigDirName);

  const WindowCfg& cfg_window = app_config_.window;
  frame_buf_ = std::make_unique<RenderTarget>(cfg_window.width, cfg_window.height);
}

bool App::init() {
  if(!createWindow()) return false;
  
  if(!loadScene()) return false;
  
  setupTools();

  shadow_renderer_.init(active_scene_->mainCam(),
                        active_scene_->mainLight(),
                        app_config_.render.shadow.shadow_map_res);

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

  FrameCtx frame_ctx {
    .main_light = active_scene_->mainLight(),
    .shadow_map = shadow_renderer_.shadowMap()
  };

  // ---------- Shadow Pass ----------
  shadow_renderer_.render(active_scene_->meshNodes(), active_scene_->worldAABB());
  frame_ctx.dir_light_shadow_texel_size_ws = shadow_renderer_.shadowTexelSizeWorld();

  // ---------- Main Pass ----------
  VariantShader active_shader = BlinnShader{};
  
  frame_buf_->clearColorBuf();
  frame_buf_->clearDepthBuf();
  
  ViewCtx view_ctx_main{ active_scene_->mainCam() };

  PassCtx pass_ctx_opaque {
    .frame_ctx = &frame_ctx,
    .view_ctx  = &view_ctx_main,
    .buffer    = frame_buf_.get(),
    .shader    = &active_shader,
  };
  
  RenderQueue opaque_queue{RenderQueue::Type::Opaque};
  opaque_queue.build(active_scene_->meshNodes(), view_ctx_main);
  opaque_queue.sort();

  Pipeline::executeCommands(opaque_queue.commands(), pass_ctx_opaque);
  
  // ---------- Post Process ----------
  frame_buf_->toneMap();

  render_stat.draw_calls += opaque_queue.commands().size();
  
  profiler_.collectStats(render_stat);

  // mfb_update_state 为用于 mfb_update_ex 返回值的枚举:
  // 1. STATE_OK 更新成功；2. STATE_EXIT 用户按了 esc，或程序请求退出；
  // 3. STATE_INVALID_WINDOW 窗口句柄无效；
  // 4. STATE_INTERNAL_ERROR minifb 内部错误；
  mfb_update_state state = mfb_update_ex(mfb_window_,
                                         frame_buf_->colorBuf(),
                                         frame_buf_->width(),
                                         frame_buf_->height());
  // 更新异常，则主动断开对窗口句柄的持有
  if (state != MFB_STATE_OK) mfb_window_ = nullptr;
}

bool App::createWindow() {
  const WindowCfg& cfg_window = app_config_.window;

  mfb_window_ = mfb_open_ex(cfg_window.title.c_str(),
                            cfg_window.width,
                            cfg_window.height,
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
  
  auto lua_path = fs::path(PROJECT_ROOT_DIR) / "scripts"
    / "scene_sponza.lua";
  
  active_scene_ = Scene::loadFromLua(lua_path, cfg_window.aspectRatio());

  if(!active_scene_) {
    fmt::print(stderr, "App - Initialize Failed\n");
    return false;
  }

  return true;
}

void App::setupTools() noexcept {
  controller_.bind(active_scene_->mainCam(), &input_);
  profiler_.setReportInterval(1.0f);
}

void App::onWindowResize() const {
  const u32 w_new = mfb_get_window_width(mfb_window_);
  const u32 h_new = mfb_get_window_height(mfb_window_);
  
  if(w_new == 0 || h_new == 0) return;

  // 注：更改 aspect ratio 后，相机视椎形状会变，进而使阴影拟合产生变化
  active_scene_->mainCam()->setAspectRatio(
    static_cast<f32>(w_new) / static_cast<f32>(h_new));
  
  frame_buf_->resize(w_new, h_new);
}

void App::quit() {
  app_config_.saveToTomlFile();
}