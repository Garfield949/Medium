///
/// @file App.h
/// @brief 负责渲染器程序的生命周期管理：初始化、循环、析构
///
#pragma once

#include "Base/Timer.h"
#include "Base/Input.h"
#include "Base/Controller.h"
#include "Pipeline/RenderTarget.h"
#include "Scene/Node.h"
#include "Scene/Light/DirectionalLight.h"
#include <memory>

// 该前向声明不能放在 namespace Medium 里，否则编译器会将类型识别为 Medium::mfb_window，
// 但 MiniFB 是在全局作用域声明的，即 ::mfb_window.
struct mfb_window;

namespace Medium {

  class Camera;

  class App {
  public:
     App() = default;
    ~App() = default;

    // 资源加载
    [[nodiscard]] bool init();

    // 主循环
    void loop();

  private:
    void on_update(f32 dt);
    void on_render();

    mfb_window *mfb_window_ = nullptr;
    Timer timer_;
    Input input_;
    std::unique_ptr<Controller> controller_ = nullptr;

    std::unique_ptr<Node>         scene_root_;
    std::unique_ptr<RenderTarget> main_buffer_ = nullptr;
    Camera*                       main_camera_ = nullptr;

    std::unique_ptr<DirectionalLight> main_light_ = nullptr;
    std::unique_ptr<RenderTarget>     shadow_map_  = nullptr;
  };

} // Medium