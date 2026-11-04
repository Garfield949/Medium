///
/// @file   SSAORenderer.h
/// @author Garfield949
/// @date   2026/11/1 10:39
/// @brief  
///
#pragma once

#include <array>

#include "Base/Types.h"
#include "Resources/Image2D.h"

namespace Medium {

class  FrameBuffer;
struct CameraContext;

/// @class SSAORenderer
class SSAORenderer {
public:
  explicit SSAORenderer();

  void render(const FrameBuffer& frame_buffer, const CameraContext& cam_ctx);

  void resize(i32 width, i32 height);

  const Image2D<f32>* ao() const noexcept { return &ao_raw_; }

  // 调整遮蔽作用半径。
  f32 radius = 0.5f;
  f32 bias = 0.01f;

private:
  void blur(const FrameBuffer& frame_buffer);
  
  // 半球采样点个数。
  static constexpr i32 kSampleCount = 32;

  // 局部 +Z 半球内的采样点，SSAORenderer 构造时生成。
  std::array<Vec3f, kSampleCount> sample_offsets;

  // 4x4 随机旋转表，SSAORenderer 构造时生成。
  std::array<Vec2f, 16> rotations;
  
  Image2D<f32> z_view_;  // 提前缓存当前帧屏幕像素的视空间重建深度，否则 render 中可能会产生重复计算。
  Image2D<f32> ao_raw_;  // 原始 ao, 存放的是可见性。
};

} // namespace Medium
