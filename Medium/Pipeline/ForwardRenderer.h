///
/// @file   ForwardRenderer.h
/// @author Garfield949
/// @date   2026/5/27 20:40
/// @brief  
///
#pragma once

#include <span>

#include "Base/Types.h"
#include "Pipeline/RenderTarget.h"
#include "Pipeline/Shader/VariantShader.h"

namespace Medium {

class Camera;
class MeshNode;
struct CSMSamplingCtx;

class ForwardRenderer {
public:
  explicit ForwardRenderer() = default;

  void init(u32 window_width, 
            u32 window_height, 
            Camera* camera, 
            Vec3f light_dir);

  void render(std::span<const MeshNode* const> scene_mesh_nodes,
              const CSMSamplingCtx* csm_data);

  RenderTarget* frameBuffer() noexcept {
    return frame_buffer_.get();
  }

private:
  std::unique_ptr<RenderTarget> frame_buffer_; // 使用智能指针是为了延迟初始化。

  const Camera* camera_ = nullptr;
  Vec3f light_dir_;

  VariantShader blinn_shader_ = BlinnShader{};
};

} // Medium
