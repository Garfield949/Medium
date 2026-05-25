///
/// @file RenderTarget.h
/// @brief 渲染对象类型，管理各种缓冲
/// @details nodiscard 为 C++17 引入的属性修饰符：当调用者未对返回值进行处理，编译器会发出一个警告。
///
#pragma once

#include "Base/Color.h"
#include <cstddef>

namespace Medium {

/// @class RenderTarget
/// @brief 渲染对象类型
/// @details 存储帧缓冲和深度缓冲
class RenderTarget {
public:
  enum class Mode { Normal, DepthOnly };
  
  static constexpr f32 kDepthClearVal = 0.0f;

  // 构造函数
  explicit RenderTarget(u32 width, u32 height, Mode mode = Mode::Normal)
    : mode_{mode} {
    resize(width, height);
  }

  explicit RenderTarget(u32 res, Mode mode) : RenderTarget{res, res, mode} {}

  void resize(u32 width_new, u32 height_new) {
    if (width_new == w_ && height_new == h_) return;
    w_ = width_new;
    h_ = height_new;
    depth_buf_.resize(w_ * h_, kDepthClearVal);
    if(mode_ == Mode::Normal) {
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

  // 视区范围是 [0, width] [0, height]，而屏幕坐标系是定义在左上角，+y 指向下的，故需翻转 y 轴
  // 缓冲区索引是从 0 开始的，故 y 值减一。
  [[nodiscard]] size_t getBufIdx(size_t x, size_t y) const noexcept {
    return (h_ - y - 1) * w_ + x;
  }

  // 深度测试，Reversed-Z 致使离 0 更近的深度对应离相机更远的顶点
  [[nodiscard]] bool depthTest(size_t index, f32 new_dep) const noexcept {
    return new_dep > depth_buf_[index];
  }

  void writeDepth(u32 index, f32 new_dep) noexcept {
    depth_buf_[index] = new_dep;
  }

  void setColor(u32 index, const LinearColor& linear) noexcept {
    if(mode_ == Mode::DepthOnly) return;
    color_buf_hdr_[index] = linear;
  }

  // TODO: 改名
  /// @brief 色调映射 & Gamma Encoding
  void toneMap() noexcept {
    if (mode_ == Mode::DepthOnly) return;
    for(size_t i = 0; i < color_buf_hdr_.size(); ++i)
      color_buf_final_[i] = color_buf_hdr_[i].toneMapped().toColor(ColorSpace::SRGB).data;
  }
  
  void postProcess() noexcept {
    toneMap();
  }

  /// @brief 适用于阴影贴图的采样
  /// @param uv 传入
  [[nodiscard]] f32 sampleDepth(const Vec2f& uv) const noexcept {
    f32 U = std::clamp(uv.x(), 0.0f, 1.0f);
    f32 V = std::clamp(uv.y(), 0.0f, 1.0f);

    f32 x_tmp = U * static_cast<f32>(w_) - 0.5f;
    f32 y_tmp = V * static_cast<f32>(h_) - 0.5f;

    size_t x = std::clamp(std::lround(x_tmp), 0l, static_cast<long>(w_ - 1));
    size_t y = std::clamp(std::lround(y_tmp), 0l, static_cast<long>(h_ - 1));

    return depth_buf_[getBufIdx(x, y)];
  }

  [[nodiscard]] u32* colorBuf() noexcept { return color_buf_final_.data(); }

  // 获取缓冲区宽/高
  [[nodiscard]] u32 width()  const noexcept { return w_; }
  [[nodiscard]] u32 height() const noexcept { return h_; }

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