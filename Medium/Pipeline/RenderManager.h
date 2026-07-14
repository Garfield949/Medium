///
/// @file   RenderManager.h
/// @author Garfield949
/// @date   2026/6/21 15:22
/// @brief  
///
#pragma once

#include "Pipeline/RenderStatus.h"
#include "Pipeline/RenderContext.h"
#include "Pipeline/FrameBuffer.h"
#include "Pipeline/SceneRenderer.h"
#include "Pipeline/DebugRenderer.h"
#include "Pipeline/Shadow/ShadowManager.h"

namespace Medium {

class Scene;

class RenderManager {
public:
  explicit RenderManager() = default;

  /// @brief 初始化。
  bool init(Scene* scene, const Vec2i& buffer_extent_physical);

  /// @brief 完成一帧阴影 pass 和主 pass 的渲染。
  void render();

  // ---------- Setters ----------
  /// @brief 绑定新场景。
  /// @details 1. 阴影管理器绑定新场景的相机、方向光、阴影配置；
  ///          2. 场景渲染器绑定新场景的相机和方向光。
  void bindNewScene(Scene* new_scene);

  /// @brief 窗口大小变化回调，调整主 pass 帧缓冲尺寸。
  bool onResize(const Vec2i& buffer_extent_physical);

  // ---------- Getters ----------
  /// @brief 返回主 pass 帧缓冲引用。
  FrameBuffer& frameBuffer() noexcept { return frame_buffer_; }

  /// @brief 返回阴影管理器 / 场景渲染器引用。
  ShadowManager& shadowManager() noexcept { return shadow_manager_; }
  SceneRenderer& sceneRenderer() noexcept { return scene_renderer_; }

  /// @brief 获取渲染统计信息。
  const RenderStatus& renderStatus() const noexcept { return render_status_; }

  // ---------- ImGui ----------
  /// @brief 设置绘制模式（着色和线框）。
  /// @details 设置 SceneRenderer 为不透明和透明队列使用指定的 DrawMode，并根据模式决定是否在 draw 中
  ///          启用 ShadowManager::render()，并通过 SceneRenderer 在 ShaderBindCtx 中设置阴影查询
  ///          标记。
  void setDrawMode(DrawMode mode) noexcept;
  /// @brief 返回绘制模式。
  /// @details 需缓存 DrawMode: Debug 面板关闭时 Scene 面板的方向光仍可以设置是否 cast shadow, 就需联
  /// 合绘制模式组合判断是否开启阴影 pass.
  DrawMode drawMode() const noexcept { return draw_mode_; }

  /// @brief 返回当前是否启用阴影功能。
  bool shadowingEnabled() const noexcept;

  /// @brief 设置是否在 HDR 缓冲上叠加一层写深度的 AABB 调试线框。
  void setDrawAABBs(bool enable) noexcept {
    draw_aabbs_ = enable;
  }

  bool drawAABBs() const noexcept {
    return draw_aabbs_;
  }

private:
  DrawMode draw_mode_ = DrawMode::Shaded;

  bool draw_aabbs_ = false;

private:
  Scene* active_scene_ = nullptr;

  FrameBuffer frame_buffer_{FrameBuffer::Type::ColorDepth};
  
  ShadowManager shadow_manager_;
  SceneRenderer scene_renderer_;
  DebugRenderer debug_renderer_;
  
  RenderStatus render_status_;
};

} // namespace Medium
