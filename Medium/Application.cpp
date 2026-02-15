///
/// @file Application.cpp
/// @brief
///
#include "Application.h"

#include <MiniFB.h>
#include <fmt/core.h>

#include "Resources/Mesh.h"
#include "Resources/Texture.h"

#include "Pipeline/Shader/Shaders.h"

#include "Scene/Camera.h"
#include "Scene/MeshInstance.h"

namespace Medium {

    Application::Application(u32 width, u32 height, const std::string& title)
        : render_config_(width, height),
          render_target_(width, height)
    {
        render_config_.title = title;
        render_config_.clear_color = {79, 147, 184};
    }

    bool Application::init() {
        // 创建 MiniFB 窗口，设置参数。
        mfb_window_ =
            mfb_open(render_config_.title.c_str(), render_config_.width_vp, render_config_.height_vp);
        if(!mfb_window_) {
            fmt::print(stderr, "Application::Init Failed::Cannot create a MiniFB Window.\n");
            return false;
        }
        mfb_show_cursor(mfb_window_, true);

        auto obj_path = std::filesystem::path(PROJECT_ROOT_DIR)
        / "assets" / "models" / "teapot" / "teapot.obj";

        // 网格类型，也负责读取模型
        // TODO: 读取失败就渲染一个蓝黑相间的正方体或其它东西
        auto mesh = Mesh::loadFromObj(obj_path);
        if(!mesh) {
            fmt::print(stderr, "Application - Initialize Failed\n");
            return false;
        }

        auto tex_path = std::filesystem::path(PROJECT_ROOT_DIR)
        / "assets" / "models" / "teapot" / "default.png";

        auto texture = Texture::loadFromFile(tex_path);
        if(!texture) {
            fmt::print(stderr, "Application - Initialize Failed\n");
            return false;
        }

        // 创建场景根结点。
        scene_root_node_ = std::make_unique<Node>("SceneRoot");

        auto mesh_instance = std::make_unique<MeshInstance>("Bunny", mesh);
        mesh_instance->setTexture(texture);
        mesh_instance->transform.setPosition({0, -0.7, 2});
        mesh_instance->transform.setScale(0.01);
        mesh_instance->transform.setRotation({0, 180, 0});

        scene_root_node_->attachChild(std::move(mesh_instance));

        camera_ = scene_root_node_->attachChild(std::make_unique<Camera>());
        camera_->setName("MainCamera");
        camera_->setAspectRatio(render_config_.vpRatio());
        camera_->setNear(0.01);
        camera_->setFOV(90);

        controller_ = std::make_unique<Controller>(camera_, &input_);

        timer_.setReportInterval(1.0f);

        return true;
    }

    void Application::loop() {
        do {
            const f32 dt = timer_.tick();

            on_update(dt); // 逻辑、状态更新

            on_render();   // 渲染

        } while(mfb_wait_sync(mfb_window_));
    }

    void Application::on_update(const f32 dt) {

        input_.update(mfb_window_);

        controller_->update(dt);

    }

    void Application::on_render() {
        // 填充背景颜色、刷新深度缓冲
        render_target_.clearColor(render_config_.clear_color);
        render_target_.clearDepth();

        SimpleShader shader;
        RenderContext context{rasterizer_, render_target_, shader, *camera_};

        // 递归地遍历并渲染子节点
        scene_root_node_->render(context);

        // MiniFB 状态类型
        mfb_update_state state
            = mfb_update_ex(mfb_window_,
                      render_target_.frameBuf(),
                            render_config_.width_vp,
                            render_config_.height_vp);

        // mfb_window 指针置空会关闭窗口。
        if (state != STATE_OK) mfb_window_ = nullptr;
    }

} // namespace Medium