///
/// @file   RenderContext.h"
/// @author Garfield949
/// @date   2026/2/13 16:45
/// @brief  该文件定义管线流动涉及的上下文类型
///
#pragma once

#include "Base/Geometry/Rect.h"
#include "Base/Types.h"
#include "Base/Geometry/Frustum.h"

namespace Medium {

class FrameBuffer;
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
  const CameraCtx*   camera_ctx = nullptr;
  const LightingCtx* lighting_ctx = nullptr;
};

enum class BlendMode {
  Replace, PremultipliedOver
};

struct RenderState {
  BlendMode blend_mode = BlendMode::Replace;
  bool enable_color_write = true;
  bool enable_depth_write = true;
  bool enable_depth_test  = true;
  Rect scissor{}; // 默认为一个非法 scissor，即不启用 scissor test.
};

// TODO: 这个 Ctx 的语义有点点奇怪。
/// @struct RenderPassCtx
struct RenderPassCtx {
  const RenderState& render_state;
  const RectF&       viewport;
  FrameBuffer&       buffer;
};

} // namespace Medium
