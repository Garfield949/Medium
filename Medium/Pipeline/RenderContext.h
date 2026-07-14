///
/// @file   RenderContext.h"
/// @author Garfield949
/// @date   2026/2/13 16:45
/// @brief  该文件定义管线流动涉及的上下文类型
///
#pragma once

#include "Base/Color.h"
#include "Base/Geometry/Rect.h"
#include "Base/Geometry/Frustum.h"
#include "Base/Types.h"
#include "Pipeline/RenderStatus.h"

namespace Medium {

class  FrameBuffer;
struct RectF;
struct CSMContext;

/// @struct CameraContext
/// @brief 相机视图相关的上下文
struct CameraContext {
  Vec3f   cam_pos;
  Mat4f   mat_view;
  Mat4f   mat_vp;
  Frustum frustum; // 用于 culling.
};

/// @struct LightingContext
struct LightingContext {
  Vec3f       light_dir;
  LinearColor light_color;
  const CSMContext* csm_ctx = nullptr;
};

/// @struct ShaderBindContext
struct ShaderBindContext {
  const CameraContext* camera_ctx = nullptr;
  const LightingContext* lighting_ctx = nullptr;
};

enum class DrawMode {
  Shaded, Wireframe
};

enum class BlendMode {
  Replace, PremultipliedOver
};

enum class CullMode {
  CullOff, CullFront, CullBack
};

enum class ColorWriteTarget {
  HDR, Present
};

struct RenderState {
  bool enable_depth_test  = true;
  bool enable_depth_write = true;
  bool enable_color_write = true;
  DrawMode  draw_mode  = DrawMode::Shaded;
  CullMode  cull_mode  = CullMode::CullBack;
  BlendMode blend_mode = BlendMode::Replace;
  ColorWriteTarget color_write_target = ColorWriteTarget::HDR;
  Rect scissor{}; // 默认为一个非法 scissor，即不启用 scissor test.
};

/// @struct RenderPassCtx
struct RenderPassCtx {
  const RenderState& render_state;
  const RectF&       viewport;
  FrameBuffer&       buffer;
  RenderStatus*      render_status = nullptr;
};

} // namespace Medium
