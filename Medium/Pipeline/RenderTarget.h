///
/// @file RenderTarget.h
/// @brief 渲染对象类型，管理各种缓冲
///
#pragma once

#include "Base/Color.h"
#include "Base/Geometry/Rect.h"
#include <cstddef>

namespace Medium {

/// @class RenderTarget
/// @brief 渲染对象类型
/// @details 存储帧缓冲和深度缓冲
class RenderTarget {
public:
  enum class Mode { FrameBuffer, ShadowMap };
  
  static constexpr f32 kDepthClearVal = 0.0f;

  explicit RenderTarget(u32 width, u32 height, Mode mode)
    : mode_{mode} { this->resize(width, height); }

  explicit RenderTarget(u32 resolution, Mode mode)
    : RenderTarget(resolution, resolution, mode) {}

  void resize(u32 new_width, u32 new_height) {
    if (new_width == w_ && new_height == h_) return;
    w_ = new_width;
    h_ = new_height;
    depth_buf_.resize(w_ * h_, kDepthClearVal);
    if(mode_ == Mode::FrameBuffer) {
      color_buf_hdr_  .resize(w_ * h_);
      color_buf_final_.resize(w_ * h_);
    }
  }

  void clearColorBuf(const LinearColor& linear = LinearColor::White()) noexcept {
    std::fill(color_buf_hdr_.begin(), color_buf_hdr_.end(), linear);
  }

  void clearDepthBuf() noexcept {
    std::fill(depth_buf_.begin(), depth_buf_.end(), kDepthClearVal);
  }

  /// @brief 获取屏幕坐标对应的视区缓冲下标。
  /// @details NDCToViewport 的逻辑是将 NDC 坐标映射到 [0, width] [0, height]，即将视区左下角作为原点；而
  ///          屏幕坐标系是定义在左上角（想象将缓冲区展开，从屏幕顶端左上角起一行一行地对应）。+y 指向下的，故需翻
  ///          转 y 轴；缓冲区索引是从 0 开始的，故 y 值减一。
  size_t getBufIdx(size_t x, size_t y) const noexcept {
    return (h_ - y - 1) * w_ + x;
  }

  // 深度测试，Reversed-Z 致使离 0 更近的深度对应离相机更远的顶点
  bool depthTest(size_t index, f32 new_dep) const noexcept {
    return new_dep > depth_buf_[index];
  }

  void writeDepth(size_t index, f32 new_dep) noexcept {
    depth_buf_[index] = new_dep;
  }

  void setColor(size_t index, const LinearColor& linear) noexcept {
    if(mode_ == Mode::ShadowMap) return;
    color_buf_hdr_[index] = linear;
  }

  void setColorPremultiplied(size_t index, const LinearColor& src_linear) noexcept {
    if(mode_ == Mode::ShadowMap) return;
    color_buf_hdr_[index] = LinearColor::premulOver(src_linear, color_buf_hdr_[index]);
  }

  // TODO: 改名
  /// @brief 色调映射 & Gamma Encoding
  void toneMap() noexcept {
    if (mode_ == Mode::ShadowMap) return;
    for(size_t i = 0; i < color_buf_hdr_.size(); ++i)
      color_buf_final_[i] = color_buf_hdr_[i].toneMapped().toColor(ColorSpace::SRGB).data;
  }
  
  void postProcess() noexcept {
    toneMap();
  }

  /// @brief 适用于阴影贴图的采样
  f32 sampleDepth(const Vec2f& uv) const noexcept {
    f32 u = std::clamp(uv.x(), 0.0f, 1.0f);
    f32 v = std::clamp(uv.y(), 0.0f, 1.0f);

    f32 x_tmp = u * static_cast<f32>(w_) - 0.5f;
    f32 y_tmp = v * static_cast<f32>(h_) - 0.5f;

    size_t x = std::clamp(std::lround(x_tmp), 0l, static_cast<long>(w_ - 1));
    size_t y = std::clamp(std::lround(y_tmp), 0l, static_cast<long>(h_ - 1));

    return depth_buf_[getBufIdx(x, y)];
  }

  f32 sampleTileDepth(const Vec2f& uv, const RectF& viewport) const noexcept {
    f32 u = std::clamp(uv.x(), 0.0f, 1.0f);
    f32 v = std::clamp(uv.y(), 0.0f, 1.0f);

    f32 x_tmp = viewport.x + u * viewport.w - 0.5f;
    f32 y_tmp = viewport.y + v * viewport.h - 0.5f;

    i32 x = static_cast<i32>(std::clamp(std::round(x_tmp), viewport.x, viewport.x + viewport.w - 1.0f));
    i32 y = static_cast<i32>(std::clamp(std::round(y_tmp), viewport.y, viewport.y + viewport.h - 1.0f));

    return depth_buf_[getBufIdx(x, y)];
  }

  u32* colorBuf() noexcept { return color_buf_final_.data(); }

  u32 width()  const noexcept { return w_; }
  f32 widthF() const noexcept { return static_cast<f32>(w_); }

  u32 height()  const noexcept { return h_; }
  f32 heightF() const noexcept { return static_cast<f32>(h_); }

private:
  u32 w_ = 0;
  u32 h_ = 0;
  Mode mode_;

  // 像素与深度缓冲
  std::vector<LinearColor> color_buf_hdr_;
  std::vector<u32>         color_buf_final_;
  std::vector<f32>         depth_buf_;
};

} // namespace Medium