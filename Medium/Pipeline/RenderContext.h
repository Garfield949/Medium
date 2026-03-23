///
/// @file   RenderContext.h"
/// @author Garfield949
/// @date   2026/2/13 16:45
/// @brief  
///
#pragma once

#include "Pipeline/Shader/ShaderType.h"

namespace Medium {

  class RenderTarget;
  class Camera;
  class DirectionalLight;
  
  struct FrameContext {
    const DirectionalLight* main_light = nullptr;
    RenderTarget* shadow_map = nullptr;
  };
  
  struct PassContext {
    RenderTarget*  buffer = nullptr;
    const Camera*  camera = nullptr;
    VariantShader* shader = nullptr;
    
    FrameContext*  ctx_frame   = nullptr;
  };

} // namespace Medium
