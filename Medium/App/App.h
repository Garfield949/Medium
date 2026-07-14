///
/// @file App.h
/// @brief 渲染器的生命周期管理。
///
#pragma once

#include "App/FPSController.h"
#include "App/WindowManager.h"
#include "Platform/Input.h"
#include "Scene/SceneManager.h"
#include "Pipeline/RenderManager.h"
#include "ImGuiImpl/ImGuiInspector.h"

namespace Medium {

class App {
public:
  explicit App() = default;

  bool init();
  void loop();
  void quit() const;

private:
  void initImGui();
  bool onProcessEvent();
  void onUpdate();
  void onRender();
  bool windowResizeEvent();

private:
  bool imgui_init_succeed_ = false;
  
private:
  Input         input_;
  FPSController controller_;

  WindowManager window_manager_;
  SceneManager  scene_manager_;
  RenderManager render_manager_;

  ImGuiInspector imgui_inspector_;
};

} // namespace Medium
