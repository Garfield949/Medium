///
/// @file   ForwardRenderer.h
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
struct CSMSamplingCtx;

class ForwardRenderer {
public:
  explicit ForwardRenderer() = default;

  [[nodiscard]]
  bool init(u32 window_width, 
            u32 window_height, 
            Camera* camera, 
            Vec3f light_dir);

  void render(std::span<const MeshNode* const> scene_mesh_nodes,
              const CSMSamplingCtx* csm_data);

  FrameBuffer* frameBuffer() noexcept {
    return &frame_buffer_;
  }

private:
  FrameBuffer frame_buffer_{FrameBuffer::Type::ColorDepth};

  const Camera* camera_ = nullptr;
  Vec3f light_dir_;

  VariantShader blinn_shader_ = BlinnShader{};
};

} // Medium
