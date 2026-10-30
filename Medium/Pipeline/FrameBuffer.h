///
/// @file   FrameBuffer.h
/// @author Garfield949
/// @date   2026/6/11 23:45
/// @brief
///
#pragma once

#include "Base/Color.h"
#include "Base/Geometry/Rect.h"
#include "Resources/Image2D.h"

namespace Medium {

class FrameBuffer {
public:
  enum class Type {
    ColorDepth, Depth
  };

  static constexpr f32 kDepthClearVal = 0.0f;

  explicit FrameBuffer(Type type) : type_{type} {}

  /// @brief 获取屏幕坐标对应的视区缓冲下标。
  /// @details NDCToViewport 的逻辑是将 NDC 坐标映射到 [0, width] [0, height]，即将视区左下角作为原点；而
  ///          屏幕坐标系是定义在左上角（想象将缓冲区展开，从屏幕顶端左上角起一行一行地对应）。+y 指向下的，故需翻
  ///          转 y 轴；缓冲区索引是从 0 开始的，故 y 值减一。
  size_t getIndex(i32 x, i32 y) const noexcept {
    return (h_ - y - 1) * w_ + x;
  }

  /// @brief 清空 HDR 缓冲。
  /// @details: 传入的 "White" 为场景线性白色，最终会被色调映射 -> Gamma Encode 成一个 0.9 的值，不是纯白。也没啥大碍，let it be...
  void clearColor(const LinearColor& linear = LinearColor::White()) { 
    assert(this->type_ == Type::ColorDepth);
    buf_HDR_.fill(linear); 
  }

  /// @brief 向 HDR 缓冲写入颜色。
  void writeHDRColor(size_t index, const LinearColor& color) noexcept {
    assert(this->type_ == Type::ColorDepth);
    buf_HDR_[index] = color;
  }

  void writeHDRColorPremult(size_t index, const LinearColor& src) noexcept {
    assert(this->type_ == Type::ColorDepth);
    LinearColor dst = buf_HDR_[index];
    writeHDRColor(index, LinearColor::PremulOver(src, dst));
  }

  void writePresentColor(size_t index, const LinearColor& color) {
    assert(this->type_ == Type::ColorDepth);
    buf_present_[index] = color.toColor(ColorSpace::SRGB);
  }

  void writePresentColorPremult(size_t index, const LinearColor& src) {
    assert(this->type_ == Type::ColorDepth);
    LinearColor dst = buf_present_[index].toLinear(ColorSpace::SRGB);
    writePresentColor(index, LinearColor::PremulOver(src, dst));
  }

  void clearDepth() { buf_depth_.fill(kDepthClearVal); }
  
  /// @brief 深度测试。
  /// @details Reversed-Z: [0 -> 离相机更远 ... -> 离相机更近 -> 1]
  bool depthTest(size_t index, f32 new_depth) const noexcept {
    return new_depth > buf_depth_[index];
  }

  void writeDepth(size_t index, f32 new_depth) noexcept {
    buf_depth_[index] = new_depth;
  }

  f32 sampleDepth(f32 u, f32 v, const RectF& viewport) const {
    assert(u >= 0 && u <= 1 && v >= 0 && v <= 1);

    f32 x = viewport.x + u * viewport.w;
    f32 y = viewport.y + v * viewport.h;

    i32 px = static_cast<i32>(std::clamp(std::floor(x), viewport.x, viewport.x + viewport.w - 1.0f));
    i32 py = static_cast<i32>(std::clamp(std::floor(y), viewport.y, viewport.y + viewport.h - 1.0f));

    return buf_depth_[getIndex(px, py)];
  }

  void resize(i32 new_width, i32 new_height) {
    buf_depth_.resize(new_width, new_height);
    
    if(type_ == Type::ColorDepth) {
      buf_HDR_.resize(new_width, new_height);
      buf_present_.resize(new_width, new_height);
    }

    w_ = new_width;
    h_ = new_height;
  }

  void resize(i32 resolution) { resize(resolution, resolution); }

  // ---------- Getters ----------
  i32 width()   const noexcept { return w_; }
  i32 height()  const noexcept { return h_; }
  f32 widthF()  const noexcept { return static_cast<f32>(w_); }
  f32 heightF() const noexcept { return static_cast<f32>(h_); }

  [[nodiscard]]
  const Image2D<f32>& depthBuffer() const { return buf_depth_; }

  [[nodiscard]]
  const Image2D<LinearColor>& hdrBuffer() const { return buf_HDR_; }

  [[nodiscard]]
  Image2D<Color>& presentBuffer() { return buf_present_; }

  Type type() const noexcept { return type_; }

private:
  Type type_;

  i32 w_ = 0;
  i32 h_ = 0;

  Image2D<f32>         buf_depth_;
  Image2D<LinearColor> buf_HDR_;      // 线性空间
  Image2D<Color>       buf_present_;  // SDL 绘制交付格式，ABGR8 packed U32
};

} // namespace Medium
