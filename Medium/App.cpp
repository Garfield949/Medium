///
/// @file App.cpp
/// @brief
///
#include "App.h"

#include <iostream>

#include <MiniFB.h>
#include <fmt/base.h>

#include "Base/Config.h"
#include "Pipeline/RenderContext.h"
#include "Pipeline/Shader/ShaderType.h"
#include "Pipeline/Shader/ShadowShader.h"
#include "Scene/Camera.h"
#include "Scene/Light/DirectionalLight.h"
#include "Scene/MeshInstance.h"
#include "Resources/Forwards.h"
#include "Resources/AssetCache.h"

namespace Medium {

  bool App::init() {

    // 创建窗口句柄
    mfb_window_ = mfb_open_ex(AppConstants::WindowTitle.c_str(), 
                              AppConstants::ScreenWidth,
                              AppConstants::ScreenHeight,
                              WF_RESIZABLE);
    mfb_show_cursor(mfb_window_, true);
    if(!mfb_window_) {
      fmt::print(stderr, "App::Init Failed::Cannot create a MiniFB Window.\n");
      return false;
    }
    
    main_buffer_ = std::make_unique<RenderTarget>(AppConstants::ScreenWidth, 
                                                  AppConstants::ScreenHeight, 
                                                  RenderPass::Full);

    auto obj_path = std::filesystem::path(PROJECT_ROOT_DIR) / "assets" / "models"
      / "snake" / "snake.obj";

    MeshPtr mesh = AssetCache::Instance().loadObj(obj_path);
    if(!mesh) {
      fmt::print(stderr, "App - Initialize Failed\n");
      return false;
    }

    // 创建场景根结点。
    scene_root_ = std::make_unique<Node>("SceneRoot");

    auto mesh_instance = scene_root_->attachChild(std::make_unique<MeshInstance>("Mesh Instance", mesh));
    mesh_instance->transform.setPosition({0, 0, 0});
    mesh_instance->transform.setScale(0.5);
    mesh_instance->transform.setRotation({0, 90, 0});
 
    main_camera_ = scene_root_->attachChild(std::make_unique<Camera>("Camera", ProjectionMode::Persp));
    main_camera_->setPerspParams(70, 0.01, AppConstants::screenRation());
    main_camera_->transform.setRotation({0, -180, 0});
    main_camera_->transform.translate({0, 0, 0});

    shadow_map_  = std::make_unique<RenderTarget>(RenderConstants::ShadowMapResDefault,
                                                  RenderConstants::ShadowMapResDefault,
                                                  RenderPass::Depth);
    
    main_light_ = std::make_unique<DirectionalLight>("Main Light");
    main_light_->setOrthoParams(0.1, 30, 20, 20);
    main_light_->transform.setPosition({0, 2.0f, 0});
    main_light_->transform.lookAt({-1.0f, 1.0f, 2.0f});

    controller_ = std::make_unique<Controller>(main_camera_, &input_);

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
    shadow_map_->clearDepth();

    VariantShader shadow_shader = ShadowShader{};

    RenderContext shadow_pass_context;
    shadow_pass_context.buffer = shadow_map_.get();
    shadow_pass_context.shader = &shadow_shader;
    shadow_pass_context.light  = main_light_.get();

    scene_root_->render(shadow_pass_context);
    
    main_buffer_->clearColor(RenderConstants::FrameBufferClearColor);
    main_buffer_->clearDepth();

    VariantShader main_shader = BlinnPhongShader{};

    RenderContext main_context;
    main_context.buffer      = main_buffer_.get();
    main_context.main_camera = main_camera_;
    main_context.shader      = &main_shader;
    main_context.light       = main_light_.get();
    main_context.shadow_map  = shadow_map_.get();

    scene_root_->render(main_context);

    // MiniFB 状态类型
    mfb_update_state state = mfb_update_ex(mfb_window_,
                                           main_buffer_->colorBuf(),
                                           main_buffer_->width(),
                                           main_buffer_->height());

    // mfb_window 指针置空会关闭窗口。
    if (state != STATE_OK) mfb_window_ = nullptr;
  }

} // namespace Medium