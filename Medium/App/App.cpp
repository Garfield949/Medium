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

#include "Base/Log.h"
#include "Scene/Scene.h"
#include "Scene/Camera.h"

namespace Medium {

void App::initImGui() {
  // 检查两类问题：
  // 1. 检查 imgui.h 和 imgui.cpp 是否同属一个发布版本;
  // 2. 检查 ImGui 内部类型的结构体布局是否正确。
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui::GetStyle().WindowRounding = 5.0f;  // 窗口圆角半径

  // 自定义渲染后端选这个接口。一般情况会是 OpenGL, DirectX 等。
  ImGui_ImplSDL3_InitForOther(window_manager_.SDLWindowHandler());
  ImGui_ImplMedium_Init();

  // 禁止 ImGui 写 imgui.ini 配置文件。
  ImGui::GetIO().IniFilename = nullptr;

  imgui_inspector_.controller     = &controller_;
  imgui_inspector_.window_manager = &window_manager_;
  imgui_inspector_.scene_manager  = &scene_manager_;
  imgui_inspector_.render_manager = &render_manager_;
}

bool App::init() {
  if(!window_manager_.init())
    return false;

  if(!scene_manager_.init(window_manager_.aspectRatio()))
    return false;

  if(!render_manager_.init(scene_manager_.activeScene(), window_manager_.physicalExtent()))
    return false;

  controller_.bind(scene_manager_.activeScene()->mainCamera(), &input_);

  // 需在 window_manager_ 初始化后调用。
  initImGui();

  imgui_init_succeed_ = true;

  return true;
}

void App::loop() {
  while(true) {
    if(!onProcessEvent()) return;
    onUpdate();
    onRender();
  }
}

bool App::onProcessEvent() {
  SDL_Event e;
  // 使用一个循环不断从事件队列中取出并消费下一个事件，非阻塞。
  while(SDL_PollEvent(&e)) {
    // 将事件先喂给 ImGui，因为它覆盖在上面。
    ImGui_ImplSDL3_ProcessEvent(&e);
    switch(e.type) {
    case SDL_EVENT_QUIT: return false;
    case SDL_EVENT_KEY_DOWN:
      if(e.key.key == SDLK_ESCAPE) return false;
      break;
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
      if(!onWindowResize()) return false;
      break;
    default: break;
    }
  }
  return true;
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

  imgui_inspector_.run();

  ImGui::Render();
  ImGui_ImplMedium_RenderDrawData(ImGui::GetDrawData(), render_manager_.frameBuffer());
  // ---------- ImGui ----------

  auto& fb = render_manager_.frameBuffer();
  window_manager_.commitBuffer(fb.presentBuffer().data(), fb.width(), fb.height());
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

  
  if(!window_manager_.onResize()) return false;
  render_manager_.onResize(extent_physical);
  scene_manager_.onResize(extent_physical);

  return true;
}

void App::quit() const {
  if(imgui_init_succeed_) {
    ImGui_ImplMedium_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
  } 
}

} // namespace Medium
