///
/// @file   ShadowManager.h
/// @author Garfield949
/// @date   2026/5/9 17:04
/// @brief  阴影渲染管理器
///
#pragma once

#include <span>

#include "Base/Geometry/Rect.h"
#include "Resources/Texture2D.h"
#include "Pipeline/FrameBuffer.h"
#include "Pipeline/RenderContext.h"
#include "Pipeline/DrawQueue.h"
#include "Pipeline/Shader/ShaderVariant.h"
#include "Pipeline/Shadow/CSM.h"

namespace Medium {

class Camera;
class DirectionalLight;
class MeshNode;
class RenderManager;
struct RenderStatus;
struct AABB3D;

/// @class ShadowManager
/// @brief 负责级联阴影的资源管理和渲染
class ShadowManager {
public:
  explicit ShadowManager() = default;

  [[nodiscard]]
  bool init(const Camera* main_camera,
            DirectionalLight* dir_light, 
            f32 max_shadow_dist,
            i32 cascade_count);
  
  /// @brief 渲染。
  void render(std::span<const MeshNode* const> scene_mesh_nodes,
              const AABB3D& bbox_scene_ws,
              RenderStatus& render_status);

  // ---------- Setters ----------
  /// @brief 绑定新的上下文。
  void bind(const Camera* camera,
            DirectionalLight* dir_light,
            f32 max_shadow_dist,
            i32 cascade_count) noexcept;

  /// @brief 设置 Cascade Tile 分辨率。
  void setCascadeTileRes(i32 res);

  // ---------- Getters ----------
  /// @brief 获得 CSM 采样的上下文数据，主要用于着色器的 Shadow Query.
  const CSMContext& CSMCtx() const noexcept {
    return csm_ctx_;
  }

  /// @brief 将 CSM Atlas 1/4 分辨率降采样并返回结果的 Texture2D 包装。
  const Image2D<Color>* CSMAtlasDebugImage() noexcept {
    updateAtlasDebugTexture();
    return &csm_atlas_debug_;
  }
  
  /// @brief 获取级联切片信息。
  const auto& cascadeSlices() const noexcept { return cascade_slices_; }
  
  /// @brief 获取单级联的分辨率。
  i32 cascadeTileRes() const noexcept { return cascade_tile_res_; }

  // ---------- ImGui ----------
  /// @brief 设置级联数量。
  void setCascadeCount(i32 count) noexcept;
  
  /// @brief 返回级联数量真值源。
  i32 cascadeCount() const noexcept { return cascade_cnt_; }

  /// @brief 设置最大阴影距离。
  void setMaxShadowDistance(f32 dist) noexcept;
  
  /// @brief 返回最大阴影距离真值源。
  f32 maxShadowDistance() const noexcept { return max_shadow_dist_; }

  /// @brief 设置是否冻结 CSM 数据。
  void freezeCSM(bool freeze) noexcept {
    if(freeze)
      csm_freeze_state_ = CSMFreezeState::Request;
    else
      csm_freeze_state_ = CSMFreezeState::UnFrozen;
  }
  
  /// @brief 返回是否已冻结 CSM 数据的真值源。
  bool CSMFrozen() const noexcept {
    return csm_freeze_state_ == CSMFreezeState::Frozen;
  }

  /// @brief 设置是否显示级联带 debug 视图。
  void enableCascadeBandView(bool enable) noexcept {
    csm_ctx_.cascade_band_view_enabled = enable;
  }

  /// @brief 返回是否已启用级联带 debug 视图的真值源。
  bool cascadeBandViewEnabled() const noexcept {
    return csm_ctx_.cascade_band_view_enabled;
  }

  /// @brief 设置是否有显示 atlas debug 视图的需求。
  void requestAtlasDebugView(bool request) noexcept {
    atlas_preview_requested_ = request;
  }

private:
  void resize(i32 tile_res);                // 受 cascade_tile_res_ 影响。
  void updateCascadeSlices()     noexcept;  // 受 cascade_cnt_ 和 max_shadow_dist_ 影响。
  void updateCascadeViewports()  noexcept;  // 受 cascade_tile_res_ 影响。
  void updateAtlasDebugTexture() noexcept;

  bool atlas_preview_requested_ = false;

  enum class CSMFreezeState {
    UnFrozen,  // Cascade 数据和 Atlas 同时更新为一致的帧内快照。
    Request,   // 当 UI 请求冻结后，必须进行一次强制的快照更新。
    Frozen
  };

  CSMFreezeState csm_freeze_state_ = CSMFreezeState::UnFrozen;

  struct CSMUpdatePolicy {
    bool update_cascade_data = true;
    bool update_atlas = true;
  };

  CSMUpdatePolicy makeUpdatePolicy() const noexcept;

private:
  ShaderVariant depth_shader_ = DepthShader{};

  FrameBuffer    csm_atlas_{FrameBuffer::Type::DepthOnly};
  Image2D<Color> csm_atlas_debug_;

  CSMContext csm_ctx_;
  std::array<f32, kMaxCSMCascades + 1> cascade_slices_;
  std::array<RectF, kMaxCSMCascades>   cascade_viewports_;

  i32 cascade_tile_res_ = 0;
  i32 cascade_cnt_      = 0;
  f32 max_shadow_dist_  = 0.0f;

  const Camera* camera_ = nullptr;
  DirectionalLight* dir_light_ = nullptr;  // 需 applyViewFit，不能是 const.
 
  RenderState opaque_render_state_;
  DrawQueue opaque_queue_{DrawQueue::Type::Opaque};
};

} // namespace Medium
