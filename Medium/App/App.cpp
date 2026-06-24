///
/// @file App.cpp
/// @brief
///
#include "App.h"

#include <fmt/base.h>
#include <SDL3/SDL_events.h>
#include <DearImGui/backend/imgui_impl_sdl3.h>

#include "ImGuiImpl/imgui_impl_medium.h"
#include "ImGuiImpl/ImGuiInspector.h"

#include "App/Config/Config.h"
#include "Base/Log.h"
#include "Scene/Scene.h"
#include "Scene/Camera.h"

namespace Medium {

void App::initImGui() {
  f32 main_scale = SDL_GetWindowDisplayScale(window_manager_.SDLWindowHandler());
  
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  
  // 绘制样式应用缩放。
  ImGui::StyleColorsDark();
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(main_scale);
  style.FontScaleDpi = main_scale;
  
  // 自定义渲染后端选这个接口。一般情况会是 OpenGL, DirectX 等。
  ImGui_ImplSDL3_InitForOther(window_manager_.SDLWindowHandler()); 
  ImGui_ImplMedium_Init();

  imgui_inspector_.controller     = &controller_;
  imgui_inspector_.window_manager = &window_manager_;
  imgui_inspector_.scene_manager  = &scene_manager_;
  imgui_inspector_.render_manager = &render_manager_;
}

bool App::init() {
  app_config_ = AppCfg::LoadFromToml(
    std::filesystem::path(PROJECT_ROOT_DIR) / Config::kConfigDirName
  );

  if(!window_manager_.init(app_config_.window))
    return false;

  if(!scene_manager_.init(app_config_.window))
    return false;

  if(!render_manager_.init(scene_manager_.activeScene(), app_config_.render, window_manager_.physicalExtent()))
    return false;

  controller_.bind(scene_manager_.activeScene()->camera(), &input_);

  initImGui();

  return true;
}

void App::loop() {
  while(true) {
    SDL_Event e;
    // 使用一个循环不断从事件队列中取出并消费下一个事件，非阻塞。
    while(SDL_PollEvent(&e)) {
      // 将事件先喂给 ImGui，因为它覆盖在上面。
      ImGui_ImplSDL3_ProcessEvent(&e);
      switch(e.type) {
      case SDL_EVENT_QUIT: return;
      case SDL_EVENT_KEY_DOWN: if(e.key.key == SDLK_ESCAPE) return; 
        break;
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: if(!onWindowResize()) return; 
        break;
      default: break;
      }
    }
    onUpdate();
    onRender();
  }
}

void App::onUpdate() {
  input_.update();
  controller_.update(ImGui::GetIO().DeltaTime);
}

void App::onRender() {
  render_manager_.render();
  
  // ---------- ImGui ----------
  ImGui_ImplMedium_NewFrame();  // Dummy.
  ImGui_ImplSDL3_NewFrame();    // SDL 后端每帧把窗口/输入状态同步给 ImGui，注释掉的话 ui 没法显示。
  ImGui::NewFrame();

  imgui_inspector_.show();
  
  ImGui::Render();
  ImGui_ImplMedium_RenderDrawData(ImGui::GetDrawData(), render_manager_.frameBuffer());
  
  // ---------- ImGui ----------
  auto& fb = render_manager_.frameBuffer();
  window_manager_.commitBuffer(fb.presentColorBuffer(), fb.width(), fb.height());
}

[[nodiscard]]
bool App::onWindowResize() {
  Vec2i extent_physical = window_manager_.physicalExtent();

  // 测试过窗口手动的最小尺寸为 1，出现 0 只能说明出现异常。
  if(extent_physical.x() <= 0 || extent_physical.y() <= 0) {
    Log::printLogPrefix("App", "onWindowResize", true);
    fmt::print("Invalid window size: [{} x {}]\n", extent_physical.x(), extent_physical.y());
    return false;
  }

  if(!render_manager_.onResize(extent_physical)) return false;

  if(!window_manager_.onResize()) return false;

  scene_manager_.onResize(extent_physical);

  return true;
}

void App::quit() const {
  ImGui_ImplMedium_Shutdown();
  app_config_.saveToTomlFile();
}

} // namespace Medium
