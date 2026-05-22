///
/// @file   RenderContext.h"
/// @author Garfield949
/// @date   2026/2/13 16:45
/// @brief  该文件定义管线流动涉及的上下文类型
///
#pragma once

#include "Base/Types.h"
#include "Scene/Camera.h"
#include "Pipeline/Shader/VariantShader.h"

namespace Medium {

class RenderTarget;
class DirectionalLight;

/// @struct FrameCtx
/// @brief 一帧所需的上下文
struct FrameCtx {
  const DirectionalLight* main_light = nullptr;
  RenderTarget* shadow_map = nullptr;

  f32 dir_light_shadow_texel_size_ws = 0.0f;
};

/// @struct ViewCtx
/// @brief 相机视图相关的上下文
struct ViewCtx {
  explicit ViewCtx(const Camera* cam)
    : view{cam->viewMatrix()},
      proj{cam->projMatrix()},
      vp{proj * view},
      frustum{cam->frustum()},
      cam_pos{cam->transform.worldPosition()} {}

  Mat4f view;
  Mat4f proj;
  Mat4f vp;
  Frustum frustum;
  Vec3f cam_pos;
};

/// @struct PassCtx
/// @brief 单 Pass 所需的上下文
struct PassCtx {
  const FrameCtx* frame_ctx;
  const ViewCtx*  view_ctx;

  RenderTarget*  buffer;
  VariantShader* shader;
};

} // namespace Medium
