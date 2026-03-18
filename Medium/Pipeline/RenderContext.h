///
/// @file   RenderContext.h
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

  struct RenderContext {
    RenderTarget     *buffer      = nullptr;
    Camera           *main_camera = nullptr;
    VariantShader    *shader      = nullptr;
    DirectionalLight *light       = nullptr;
    RenderTarget     *shadow_map  = nullptr;
  };

} // namespace Medium
