///
/// @file App.cpp
/// @brief
///
#include "App.h"

#include <iostream>
#include <MiniFB.h>
#include <fmt/base.h>

#include "Base/Types.h"
#include "Geometry/AABB.h"
#include "Geometry/Geometry.h"

#include "Pipeline/RenderTarget.h"
#include "Pipeline/Shader/BlinnShader.h"
#include "Pipeline/Shader/DepthShader.h"

#include "Scene/Scene.h"
#include "Scene/MeshNode.h"
#include "Scene/Light/DirectionalLight.h"

#include "Pipeline/Pipeline.h"
#include "Pipeline/ShadowMap.h"
#include "Pipeline/RenderContext.h"
#include "Pipeline/RenderQueue.h"
#include "Pipeline/Shader/Shader.h"

namespace Medium {

  bool App::init() {
    app_config_ = loadAppConfig(std::filesystem::path(PROJECT_ROOT_DIR)/"config");

    const WindowCfg& cfg_window = app_config_.window;
    const RenderCfg& cfg_render = app_config_.render;

    mfb_window_ = mfb_open_ex(cfg_window.title.c_str(),
                              cfg_window.width,
                              cfg_window.height,
                              WF_RESIZABLE);
    if(!mfb_window_) {
      fmt::print(stderr, "App::Init Failed::Cannot create a MiniFB Window.\n");
      return false;
    }
    mfb_show_cursor(mfb_window_, true);
    
    auto lua_path = std::filesystem::path(PROJECT_ROOT_DIR) / "scripts"
      / "scene_sponza.lua";
    
    auto scene = Scene::loadFromLua(lua_path, cfg_window.aspectRatio());
    if(scene) active_scene_ = std::move(scene);
    else {
      fmt::print(stderr, "App - Initialize Failed\n");
      return false;
    }

    controller_.bind(active_scene_->mainCam(), &input_);

    profiler_.setReportInterval(1.0f);

    frame_buf_  = std::make_unique<RenderTarget>(cfg_window.width, cfg_window.height);
    shadow_map_ = std::make_unique<RenderTarget>(cfg_render.shadow_map_res, RenderTarget::Mode::DepthOnly);

    return true;
  }

  void App::loop() {
    do {
      const f32 dt = frame_timer_.tick();

      on_update(dt);
      
      on_window_resize();

      on_render();

      // 若句柄因错误被置空，mfb_wait_sync 返回 false
    } while(mfb_wait_sync(mfb_window_));
  }

  void App::on_update(const f32 dt) {
    input_.update(mfb_window_);
    profiler_.update(dt);
    controller_.update(dt);
  }

  void App::on_render() {
    RenderStat render_stat{};
    VariantShader active_shader;

    FrameCtx frame_ctx {
      active_scene_->mainLight(),
      shadow_map_.get()
    };

    // ---------- Shadow Pass ----------

    // AABB3D bbox_scene;
    // active_scene_->root()->traverse([&](Node* node) {
    //   if(const MeshNode* mesh_node = Node::cast_to<MeshNode>(node)) {
    //     bbox_scene.merge(mesh_node->aabb());
    //   }
    // });
    
    DirectionalShadowFitDesc desc{
      active_scene_->mainCam(),
      active_scene_->mainLight(),
      app_config_.render.shadow_map_res,
      30.0f
    };

    const auto fit = ShadowMap::directionalShadowFit(desc);
    ShadowMap::applyDirectionalShadowFit(active_scene_->mainLight(), fit);

    shadow_map_->clearDepthBuf();

    active_shader = DepthShader{};

    ViewCtx view_ctx_shadow { active_scene_->mainLight()->attachedCamera() };

    PassCtx pass_ctx_shadow {
      &frame_ctx,
      &view_ctx_shadow,
      shadow_map_.get(),
      &active_shader,
    };

    RenderQueue shadow_caster_q{RenderQueue::Type::Shadow};
    shadow_caster_q.build(active_scene_->root(), view_ctx_shadow);
    shadow_caster_q.sort();

    Pipeline::executeCommands(shadow_caster_q.commands(), pass_ctx_shadow);

    // ---------- Main Pass ----------
    frame_buf_->clearColorBuf();
    frame_buf_->clearDepthBuf();
    
    active_shader = BlinnShader{};

    ViewCtx view_ctx_main{ active_scene_->mainCam() };

    PassCtx pass_ctx_opaque {
      &frame_ctx,
      &view_ctx_main,
      frame_buf_.get(),
      &active_shader,
    };
    
    RenderQueue opaque_q{RenderQueue::Type::Opaque};
    opaque_q.build(active_scene_->root(), view_ctx_main);
    opaque_q.sort();

    Pipeline::executeCommands(opaque_q.commands(), pass_ctx_opaque);
    
    // ---------- Post Process ----------
    frame_buf_->toneMap();

    render_stat.draw_calls += opaque_q.commands().size();
    
    profiler_.collectStats(render_stat);

    // mfb_update_state 为用于 mfb_update_ex 返回值的枚举:
    // 1. STATE_OK 更新成功；2. STATE_EXIT 用户按了 esc，或程序请求退出；
    // 3. STATE_INVALID_WINDOW 窗口句柄无效；
    // 4. STATE_INTERNAL_ERROR minifb 内部错误；
    mfb_update_state state = mfb_update_ex(mfb_window_,
                                           frame_buf_->colorBuf(),
                                           frame_buf_->width(),
                                           frame_buf_->height());
    // 更新异常，则主动断开对窗口句柄的持有
    if (state != STATE_OK) mfb_window_ = nullptr;
  }
  
  void App::on_window_resize() {
    const u32 width_new  = mfb_get_window_width(mfb_window_);
    const u32 height_new = mfb_get_window_height(mfb_window_);
    
    if(width_new == 0 || height_new == 0) return;
    
    active_scene_->mainCam()->setAspectRatio(
      static_cast<f32>(width_new) / static_cast<f32>(height_new));
    
    frame_buf_->resize(width_new, height_new);
  }

} // namespace Medium