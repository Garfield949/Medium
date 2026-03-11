///
/// @file App.cpp
/// @brief
///
#include "App.h"

#include <MiniFB.h>
#include <fmt/core.h>

#include "Scene/Camera.h"
#include "Scene/MeshInstance.h"
#include "Pipeline/Shader/ShaderType.h"
#include "Resources/AssetCache.h"

namespace Medium {

  bool App::init() {
    // 创建 MiniFB 窗口，设置参数。
    mfb_window_ =
      mfb_open_ex(AppConstants::WindowTitle.c_str(), AppConstants::ViewportWidth, AppConstants::ViewportHeight, WF_RESIZABLE);
    if(!mfb_window_) {
      fmt::print(stderr, "App::Init Failed::Cannot create a MiniFB Window.\n");
      return false;
    }
    mfb_show_cursor(mfb_window_, true);

    auto obj_path = std::filesystem::path(PROJECT_ROOT_DIR) / "assets" / "models"
      / "conference" / "conference.obj";

    auto mesh = AssetCache::Instance().loadObj(obj_path);
    if(!mesh) {
      fmt::print(stderr, "App - Initialize Failed\n");
      return false;
    }

    // 创建场景根结点。
    scene_root_ = std::make_unique<Node>("SceneRoot");

    auto mesh_instance = std::make_unique<MeshInstance>("Mesh Instance", mesh);
    mesh_instance->transform.setPosition({0, 0, 0});
    mesh_instance->transform.setScale(0.006);
    mesh_instance->transform.setRotation({0, -90, 0});

    scene_root_->attachChild(std::move(mesh_instance));

    camera_ = scene_root_->attachChild(std::make_unique<Camera>());
    camera_->setName("MainCamera");
    camera_->setAspectRatio(AppConstants::getScreenRatio());
    camera_->setNear(0.01);
    camera_->setFOV(85);
    camera_->transform.setRotation({0, 90, 0});
    camera_->transform.translate({0, 0, 0});

    controller_ = std::make_unique<Controller>(camera_, &input_);

    timer_.setReportInterval(1.0f);

    return true;
  }

  void App::loop() {
    do {
      const f32 dt = timer_.tick();

      on_update(dt); // 逻辑、状态更新

      on_render();   // 渲染

    } while(mfb_wait_sync(mfb_window_));
  }

  void App::on_update(const f32 dt) {

    input_.update(mfb_window_);

    controller_->update(dt);

  }

  void App::on_render() {
    // 填充背景颜色、刷新深度缓冲
    render_target_.clearFrameBuffer(RenderConstants::FrameBufferClearColor);
    render_target_.clearDepthBuffer();

    VariantShader shader = BlinnPhongShader{};

    RenderContext context{render_target_, shader, *camera_};
    context.projection_view = camera_->projMatrix() * camera_->viewMatrix();

    // 递归地遍历并渲染子节点
    scene_root_->render(context);

    // MiniFB 状态类型
    mfb_update_state state
      = mfb_update_ex(mfb_window_,
              render_target_.frameBuf(),
              render_target_.width(),
              render_target_.height());

    // mfb_window 指针置空会关闭窗口。
    if (state != STATE_OK) mfb_window_ = nullptr;
  }

} // namespace Medium