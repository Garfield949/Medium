///
/// @file   ShadowManager.h
/// @author Garfield949
/// @date   2026/5/9 17:04
/// @brief  
///
#pragma once

#include <span>

#include "Pipeline/RenderTarget.h"
#include "Pipeline/Shader/VariantShader.h"
#include "Pipeline/Shadow/DirectionalViewFit.h"

namespace Medium {

class Camera;
class DirectionalLight;
class MeshNode;

struct DirectionalViewFitDesc;

class ShadowManager {
public:
  explicit ShadowManager() = default;

  void init(Camera* main_camera, DirectionalLight* main_light, u32 shadow_map_res) {
    this->main_camera_ = main_camera;
    this->main_light_  = main_light;
    shadow_map_ = std::make_unique<RenderTarget>(shadow_map_res, RenderTarget::Mode::DepthOnly);
  }

  void render(std::span<const MeshNode* const> mesh_nodes,
              const AABB3D& bbox_scene_ws);

  [[nodiscard]]
  RenderTarget* shadowMap() {
    return shadow_map_.get();
  }

  [[nodiscard]]
  f32 shadowTexelSizeWorld() const noexcept {
    return shadow_texel_size_ws_;
  }

private:
  std::unique_ptr<RenderTarget> shadow_map_ = nullptr;
  VariantShader depth_shader_ = DepthShader{};

  Camera*           main_camera_ = nullptr;
  DirectionalLight* main_light_  = nullptr;

  f32 shadow_texel_size_ws_ = 0.0f;
};

} // namespace Medium
