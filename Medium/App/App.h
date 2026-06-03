///
/// @file App.h
/// @brief 负责渲染器程序的生命周期管理：初始化、循环、析构
///
#pragma once

#include "App/Profiler.h"
#include "App/FPSController.h"
#include "Config/Config.h"
#include "Platform/Timer.h"
#include "Platform/Input.h"
#include "Scene/Scene.h"
#include "Pipeline/Shadow/ShadowManager.h"
#include "Pipeline/ForwardRenderer.h"

using namespace Medium;

// 该前向声明不能放在 namespace Medium 里，否则编译器会将类型识别为 Medium::mfb_window，
// 但 MiniFB 是在全局作用域声明的，即 ::mfb_window.
struct mfb_window;

class App {
public:
  explicit App() = default;

  // 资源加载
  bool init();

  // 主循环
  void loop();

  void quit();

private:
  void loadConfig();
  bool createWindow();
  bool loadScene();
  void setupTools() noexcept;

  void onWindowResize();
  void onUpdate(f32 dt);
  void onRender();
  
  AppCfg app_config_;

  mfb_window*   mfb_window_ = nullptr;
  Timer         frame_timer_;
  Input         input_;
  Profiler      profiler_;
  FPSController controller_;

  // 使用智能指针的原因：若有场景切换需求，需从 Scene::loadFromLua 接收一个 std::unique_ptr<Scene>，避免深拷贝。
  std::unique_ptr<Scene> active_scene_ = nullptr;

  ShadowManager   shadow_manager_;
  ForwardRenderer forward_renderer_;
};