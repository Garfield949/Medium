///
/// @file   SceneRenderer.h
/// @author Garfield949
/// @date   2026/5/27 20:40
/// @brief  场景渲染管理器
///
#pragma once

#include <span>

#include "Pipeline/RenderContext.h"
#include "Pipeline/DrawQueue.h"
#include "Pipeline/Shader/ShaderVariant.h"

namespace Medium {
class Camera;
class MeshNode;
class DirectionalLight;
class RenderManager;
class FrameBuffer;
struct RenderStatus;
struct CSMContext;
}

namespace Medium {

struct SceneDrawListView {
  std::span<const DrawCommand> opaque;
  std::span<const DrawCommand> transparent;
};

class SceneRenderer {
public:
  explicit SceneRenderer() = default;

  [[nodiscard]]
  bool init(const Camera* camera, const DirectionalLight* dir_light);

  /// @brief 绑定新的上下文。
  void bind(const Camera* camera, const DirectionalLight* dir_light) noexcept;

  /// @brief 渲染场景。
  void render(std::span<const MeshNode* const> scene_mesh_nodes,
              const CSMContext* csm_ctx,
              FrameBuffer&  frame_buffer,
              RenderStatus& render_status);

  // ---------- Setters ----------
  /// @brief 设置不透明和透明 pass 走哪个绘制分支。
  void setDrawMode(DrawMode mode) noexcept {
    opaque_render_state_.draw_mode = mode;
    transp_render_state_.draw_mode = mode;
  }

  // ---------- Getters ----------
  SceneDrawListView drawListView() const noexcept {
    return {
      .opaque      = opaque_queue_.drawCommands(),
      .transparent = transp_queue_.drawCommands()
    };
  }

private:
  ShaderVariant blinn_shader_ = BlinnShader{};

  const Camera* main_camera_ = nullptr;
  const DirectionalLight* dir_light_ = nullptr;

  RenderState opaque_render_state_;
  RenderState transp_render_state_;

  DrawQueue opaque_queue_{DrawQueue::Type::Opaque};
  DrawQueue transp_queue_{DrawQueue::Type::Transparent};
};

} // Medium
