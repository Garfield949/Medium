///
/// @file App.h
/// @brief 渲染器的生命周期管理。
///
#pragma once

#include "Config/Config.h"
#include "App/Profiler.h"
#include "App/FPSController.h"
#include "App/WindowManager.h"
#include "Platform/Timer.h"
#include "Platform/Input.h"
#include "Pipeline/Shadow/ShadowManager.h"
#include "Pipeline/ForwardRenderer.h"
#include "Scene/Scene.h" // std::unique_ptr 需在编译期获取类型大小。

namespace Medium {

class App {
public:
  explicit App() = default;

  bool init();
  void loop();
  void quit();

private:
  void loadConfig();
  bool loadScene();
  void setupTools() noexcept;

  bool onWindowResize();
  void onUpdate(f32 dt);
  void onRender();

  AppCfg        app_config_;
  Timer         frame_timer_;
  Input         input_;
  Profiler      profiler_;
  FPSController controller_;

  // 使用智能指针的原因：若有场景切换需求，需从 Scene::loadFromLua 接收一个 std::unique_ptr<Scene>.
  std::unique_ptr<Scene> active_scene_ = nullptr;

  WindowManager   window_manager_;
  ShadowManager   shadow_manager_;
  ForwardRenderer forward_renderer_;
};

} // namespace Medium