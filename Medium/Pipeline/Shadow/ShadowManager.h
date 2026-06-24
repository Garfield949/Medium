///
/// @file   ShadowManager.h
/// @author Garfield949
/// @date   2026/5/9 17:04
/// @brief  
///
#pragma once

#include <span>

#include "Base/Geometry/Rect.h"
#include "Pipeline/FrameBuffer.h"
#include "Pipeline/Shader/VariantShader.h"
#include "Pipeline/Shadow/CSM.h"

namespace Medium {

class Camera;
class DirectionalLight;
class MeshNode;
struct RenderStat;
struct AABB3D;

/// @class ShadowManager
/// @brief 负责级联阴影的资源管理和渲染
class ShadowManager {
public:
  explicit ShadowManager() = default;

  [[nodiscard]]
  bool init(Camera* camera,
            DirectionalLight* dir_light, 
            u32 csm_tile_res,
            f32 max_shadow_dist,
            u32 cascade_count);

  void attach(Camera* camera,
              DirectionalLight* dir_light,
              f32 max_shadow_dist,
              u32 cascade_count) noexcept;

  void updateCSMSlices(f32 max_shadow_dist, u32 cascade_count) noexcept;

  void render(std::span<const MeshNode* const> scene_mesh_nodes,
              const AABB3D& bbox_scene_ws,
              RenderStat& stat);

  const CSMSamplingCtx* CSMData() const noexcept {
    return &csm_ctx_;
  }

private:
  FrameBuffer shadow_map_atlas_{FrameBuffer::Type::DepthOnly};
  
  std::array<f32, kMaxCSMCascades + 1> csm_slices_;
  std::array<RectF, kMaxCSMCascades>   cascade_viewports_;

  CSMSamplingCtx csm_ctx_;
  
  u32 cascade_cnt_     = 0;
  f32 max_shadow_dist_ = 0.0f;
  u32 csm_tile_res_    = 0;

  Camera*           camera_    = nullptr;
  DirectionalLight* dir_light_ = nullptr;

  VariantShader depth_shader_  = DepthShader{};
};

} // namespace Medium
