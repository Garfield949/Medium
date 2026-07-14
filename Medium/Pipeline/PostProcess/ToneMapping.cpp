///
/// @file   ToneMapping.cpp
/// @author Garfield949
/// @date   2026/7/6 13:41
/// @brief  
///
#include "ToneMapping.h"

#include "Base/Color.h"
#include "Pipeline/FrameBuffer.h"

namespace Medium {
namespace {

/// @brief ACES 色调映射拟合
/// @details 将各分量的高动态范围压缩到 [0, 1] LDR 范围内。
/// @note    严格来说，线性颜色经色调映射后就不再是线性的了，语义上偷了点懒。
constexpr LinearColor ACESMethod(const LinearColor& src) noexcept {
  constexpr f32 a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
  auto formula = [&](f32 channel) {
    channel = std::max(0.0f, channel);
    return channel * (a * channel + b) / (channel * (c * channel + d) + e);
  };
  return {formula(src.R), formula(src.G), formula(src.B), src.A};
}

} // 匿名命名空间，控制仅单文件可见
} // namespace Medium

namespace Medium::PostProcess {

void toneMapping(FrameBuffer& frame_buffer) noexcept {
  assert(frame_buffer.type() == FrameBuffer::Type::ColorDepth);

  const Texture2D& buf_HDR     = frame_buffer.hdrBuffer();
  Texture2D&       buf_present = frame_buffer.presentBuffer();

  for(size_t i = 0; i < buf_present.pixelCount(); ++i) {
    LinearColor linear = buf_HDR.readLinearColor(i);

    LinearColor tone_mapped = ACESMethod(linear);

    u32 abgr8 = tone_mapped.toColor(ColorSpace::SRGB).data;

    buf_present.writePackedAbgr(i, abgr8);
  }
}

} // namespace Medium::PostProcess