///
/// @file   SceneRenderer.h
/// @author Garfield949
/// @date   2026/5/27 20:40
/// @brief  
///
#pragma once

#include <span>

#include "Base/Types.h"
#include "Pipeline/FrameBuffer.h"
#include "Pipeline/Shader/VariantShader.h"

namespace Medium {

class Camera;
class MeshNode;
struct RenderStat;

struct CSMSamplingCtx;

class SceneRenderer {
public:
  explicit SceneRenderer() = default;

  [[nodiscard]]
  bool init(const Vec2i& buffer_extent_physical, Camera* camera, const Vec3f& light_dir);

  void bind(Camera* camera, const Vec3f& light_dir) noexcept;

  void render(std::span<const MeshNode* const> scene_mesh_nodes,
              const CSMSamplingCtx* csm_data,
              RenderStat& stat);

  FrameBuffer& frameBuffer() noexcept {
    return frame_buffer_;
  }

private:
  FrameBuffer frame_buffer_{FrameBuffer::Type::ColorDepth};

  const Camera* camera_ = nullptr;
  Vec3f light_dir_;

  VariantShader blinn_shader_ = BlinnShader{};
};

} // Medium
