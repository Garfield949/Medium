///
/// @file App.h
/// @brief 负责渲染器程序的生命周期管理：初始化、循环、析构
///
#pragma once

#include "Platform/Timer.h"
#include "Platform/Profiler.h"
#include "Platform/Input.h"

#include "Pipeline/RenderTarget.h"

#include "Scene/Scene.h"
#include "Scene/FPSController.h"

// 该前向声明不能放在 namespace Medium 里，否则编译器会将类型识别为 Medium::mfb_window，
// 但 MiniFB 是在全局作用域声明的，即 ::mfb_window.
struct mfb_window;

namespace Medium {

  class App {
  public:
     App() = default;
    ~App() = default;

    // 资源加载
    [[nodiscard]] bool init();

    // 主循环
    void loop();

  private:
    void on_window_resize();
  
    void on_update(f32 dt);

    void on_render();

    mfb_window*   mfb_window_ = nullptr;
    Timer         timer_;
    Input         input_;
    Profiler      profiler_;
    FPSController controller_;

    std::unique_ptr<RenderTarget> frame_buf_  = nullptr;
    std::unique_ptr<RenderTarget> shadow_map_ = nullptr;
    std::unique_ptr<Scene>        active_scene_ = nullptr;
  };

} // Medium