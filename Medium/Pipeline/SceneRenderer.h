///
/// @file   SceneRenderer.h
/// @author Garfield949
/// @date   2026/5/27 20:40
/// @brief  场景渲染管理器
///
#pragma once

#include "Pipeline/RenderContext.h"
#include "Pipeline/Shader/BlinnShader.h"
#include "Pipeline/Shader/ShaderVariant.h"
#include "Resources/Image2D.h"

namespace Medium {

class FrameBuffer;
class DirectionalLight;

struct SkyBox;
struct CSMContext;
struct RenderStatus;
struct CameraContext;
struct SceneDrawListView;

class SceneRenderer {
public:
  explicit SceneRenderer();

  void render(const SceneDrawListView& draw_list_view,
              const CameraContext&     camera_ctx,
              const DirectionalLight*  dir_light,
              const CSMContext*        csm_ctx,
              const Image2D<f32>*      ao,
              const SkyBox*            skybox,
              FrameBuffer&             frame_buffer,
              RenderStatus&            render_status);

  /// @brief 主要控制不透明和透明 pass 走哪个绘制分支。
  void setDrawMode(DrawMode mode) noexcept;

  BlinnShader* blinnShader() { return &std::get<BlinnShader>(blinn_shader_); }

private:
  ShaderVariant blinn_shader_ = BlinnShader{};

  RenderState opaque_render_state_;
  RenderState transp_render_state_;
};

} // Medium
