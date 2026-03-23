///
/// @file App.cpp
/// @brief
///
#include "App.h"

#include <iostream>

#include <MiniFB.h>
#include <fmt/base.h>

#include "App/Config.h"
#include "Geometry/Frustum.h"

#include "Scene/Scene.h"
#include "Scene/Camera.h"
#include "Scene/Light/DirectionalLight.h"

#include "Pipeline/Pipeline.h"
#include "Pipeline/RenderContext.h"
#include "Pipeline/RenderQueue.h"
#include "Pipeline/Shader/ShaderType.h"

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

    auto lua_path = std::filesystem::path(PROJECT_ROOT_DIR) / "scripts"
      / "scene_sponza.lua";

    auto scene = Scene::loadFromLua(lua_path);
    if(scene) active_scene_ = std::move(scene);
    else {
      fmt::print(stderr, "App - Initialize Failed\n");
      return false;
    }

    controller_.bind(active_scene_->mainCam(), &input_);

    profiler_.setReportInterval(1.0f);

    shadow_map_ = std::make_unique<RenderTarget>(RenderConstants::ShadowMapResDefault,
                                                 RenderConstants::ShadowMapResDefault,
                                                 RenderPass::Depth);

    return true;
  }

  void App::loop() {
    do {
      const f32 dt = timer_.tick();

      on_update(dt);

      on_render();

    } while(mfb_wait_sync(mfb_window_));
  }

  void App::on_update(const f32 dt) {
    input_.update(mfb_window_);
    profiler_.update(dt);
    controller_.update(dt);
  }

  void App::on_render() {
    static RenderStat render_stat;
    render_stat.clear();

    RenderQueue render_queue;
    render_queue.clear();

    FrameContext ctx_frame {
      active_scene_->mainLight(),
      nullptr
    };

    // VariantShader shadow_shader = ShadowShader{};
    //
    // shadow_map_->clearDepth();
    //
    // PassContext ctx_shadow_pass {
    //   shadow_map_.get(),
    //   active_scene_->mainLight()->attachedCamera(),
    //   &shadow_shader,
    //   &ctx_frame
    // };
    //
    // active_scene_->root().commitCommand(render_queue, active_scene_->mainLight()->frustum());
    // render_stat.draw_calls += render_queue.opaqueQueue().size();
    //
    // Pipeline::executeQueue(ctx_shadow_pass, render_queue.opaqueQueue());
    //
    // ctx_frame.shadow_map = shadow_map_.get();
    //
    // render_queue.clear();
    
    frame_buffer_.clearColor();
    frame_buffer_.clearDepth();

    VariantShader main_shader = BlinnPhongShader{};

    PassContext ctx_main_pass {
      &frame_buffer_,
      active_scene_->mainCam(),
      &main_shader,
      &ctx_frame
    };

    active_scene_->root().commitCommand(render_queue, active_scene_->mainCam()->frustum());

    render_stat.draw_calls += render_queue.opaqueQueue().size();
    profiler_.collectStats(render_stat);
    
    Pipeline::executeQueue(ctx_main_pass, render_queue.opaqueQueue());

    // MiniFB 状态类型
    mfb_update_state state = mfb_update_ex(mfb_window_,
                                           frame_buffer_.colorBuf(),
                                           frame_buffer_.width(),
                                           frame_buffer_.height());

    // mfb_window 指针置空会关闭窗口。
    if (state != STATE_OK) mfb_window_ = nullptr;
  }

} // namespace Medium