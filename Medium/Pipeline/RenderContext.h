///
/// @file   RenderContext.h"
/// @author Garfield949
/// @date   2026/2/13 16:45
/// @brief  该文件定义管线流动涉及的上下文类型
///
#pragma once

#include "Base/Types.h"
#include "Base/Geometry/Frustum.h"

namespace Medium {

class RenderTarget;
class DirectionalLight;
struct RectF;
struct CSMSamplingCtx;

/// @struct CameraCtx
/// @brief 相机视图相关的上下文
struct CameraCtx {
  Vec3f   cam_pos;
  Mat4f   view;
  Mat4f   vp;
  Frustum frustum; // 用于 culling.
};

/// @struct LightingCtx
struct LightingCtx {
  Vec3f light_dir;
  const CSMSamplingCtx* csm_data = nullptr;
};

/// @struct ShaderBindCtx
struct ShaderBindCtx {
  const CameraCtx&   camera_ctx;
  const LightingCtx* lighting_ctx = nullptr;
};

enum class BlendMode {
  Replace, PremultipliedOver
};

struct RenderState {
  BlendMode blend_mode = BlendMode::Replace;
  bool depth_write = true;
};

/// @struct RenderPassCtx
struct RenderPassCtx {
  const RenderState& render_state;
  const RectF&       viewport;
  RenderTarget&      buffer;
};

// 注：将某些字段设为一个指针，当目标着色器用不到该字段时，可直接设为 nullptr.

} // namespace Medium
