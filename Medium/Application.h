///
/// @file Application.h
/// @brief 负责渲染器程序的生命周期管理：初始化、循环、关闭
///
#pragma once

#include "Core/Timer.h"
#include "Pipeline/Rasterizer.h"
#include "Pipeline/RenderTarget.h"
#include "Core/Input.h"
#include "Core/Controller.h"
#include "Scene/Node.h"

// 该前向声明不能放在 namespace Medium 里，否则编译器会将类型识别为 Medium::mfb_window，但
// MiniFB 是在全局作用域声明的，即 ::mfb_window.
struct mfb_window;

namespace Medium {
    class Application {
    public:
        Application(u32 width, u32 height, const std::string & title);
        ~Application() = default;

        // 资源加载
        [[nodiscard]] bool init();

        // 主循环
        void loop();

    private:
        void on_update(f32 dt);
        void on_render();

        // 窗口与核心配置
        mfb_window *mfb_window_ = nullptr;
        RenderConfig render_config_;
        Timer timer_;

        // 渲染管线
        RenderTarget render_target_;
        Rasterizer rasterizer_;

        // 场景
        std::unique_ptr<Node> scene_root_node_;
        Camera *camera_ = nullptr;

        // 输入与控制
        Input input_;
        // Controller 的构造依赖 Camera 和 Input，前两者需设置参数，所以可实现延迟构造。
        std::unique_ptr<Controller> controller_;
    }; // Class Application

} // Medium