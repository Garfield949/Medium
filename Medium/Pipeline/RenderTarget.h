///
/// @file RenderTarget.h
/// @brief 渲染对象类型，管理各种缓冲
/// @details nodiscard 为 C++17 引入的属性修饰符：当调用者未对返回值进行处理，编译器会发出一个警告
///
#pragma once

#include "Base/Color.h"
#include "Base/Config.h"

namespace Medium {

  /// @class RenderTarget
  /// @brief 渲染对象类型
  /// @details 存储帧缓冲和深度缓冲，vp 指代 Viewport
  class RenderTarget {
  public:
    // 构造函数
    explicit RenderTarget()
      : w_(AppConstants::ViewportWidth),
        h_(AppConstants::ViewportHeight),
        frameBuf_(w_ * h_, RenderConstants::FrameBufferClearColor.toColor(ColorSpace::SRGB).data),
        depthBuf_(w_ * h_, RenderConstants::DepthClearValue) {}

    // 刷新像素与深度缓冲
    void clearFrameBuffer(const LinearColor& linear) noexcept {
      std::fill(frameBuf_.begin(), frameBuf_.end(), linear.toColor(ColorSpace::SRGB).data);
    }

    void clearDepthBuffer() noexcept {
      std::fill(depthBuf_.begin(), depthBuf_.end(), RenderConstants::DepthClearValue);
    }

    // 视区范围是 [0, width][0, height]，而屏幕坐标系是定义在左上角，+y 指向下的，故需翻转 y 轴
    // 缓冲区索引是从 0 开始的，故 y 值减一。
    [[nodiscard]] size_t getBufferIndex(u32 x, u32 y) const noexcept {
      return (h_ - y - 1) * w_ + x;
    }

    // 放置像素
    void setColor(u32 index, const LinearColor& linear) noexcept {
      frameBuf_[index] = linear.toneMapped().toColor(ColorSpace::SRGB).data;
    }

    // 深度测试
    [[nodiscard]] bool depthTest(u32 index, f32 new_dep) const noexcept {
      return new_dep > depthBuf_[index];
    }
    
    // 深度写入
    void writeDepth(u32 index, f32 new_dep) noexcept {
      depthBuf_[index] = new_dep;
    }
    
    // 返回像素缓冲的只读指针
    [[nodiscard]] u32* frameBuf() noexcept { return frameBuf_.data(); }

    // 获取缓冲区宽/高
    [[nodiscard]] u32 width()  const noexcept { return w_; }
    [[nodiscard]] u32 height() const noexcept { return h_; }

  private:
    // 缓冲区宽高
    u32 w_;
    u32 h_;

    // 像素与深度缓冲
    std::vector<u32> frameBuf_;
    std::vector<f32> depthBuf_;
  }; // class RenderTarget

} // namespace Medium