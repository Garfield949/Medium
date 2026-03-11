///
/// @file   RenderContext.h
/// @author Garfield949
/// @date   2026/2/13 16:45
/// @brief  
///
#pragma once

#include "Base/Types.h"
#include "Pipeline/Shader/ShaderType.h"

namespace Medium {

  class RenderTarget;
  class Camera;

  struct RenderContext {
    RenderContext(RenderTarget &t, VariantShader &s, Camera &c)
      : render_target(t), shader(s), camera(c) {}

    RenderTarget  &render_target;
    VariantShader &shader;
    Camera        &camera;
    
    Mat4f projection_view;
  };

} // namespace Medium
