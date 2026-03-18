///
/// @file RenderTarget.h
/// @brief 渲染对象类型，管理各种缓冲
/// @details nodiscard 为 C++17 引入的属性修饰符：当调用者未对返回值进行处理，编译器会发出一个警告
///
#pragma once

#include "Base/Color.h"
#include "Base/Config.h"

#include <fmt/base.h>

namespace Medium {
  
  enum class RenderPass { Full, Depth };

  /// @class RenderTarget
  /// @brief 渲染对象类型
  /// @details 存储帧缓冲和深度缓冲
  class RenderTarget {
  public:
    // 构造函数
    explicit RenderTarget(u32 width, u32 height, RenderPass type)
      : w_(width), 
        h_(height), 
        pass_type_(type),
        depth_buf_(w_ * h_, RenderConstants::DepthClearValue)
    {
      if(pass_type_ == RenderPass::Full)
        color_buf_.resize(w_ * h_, RenderConstants::FrameBufferClearColor.toColor(ColorSpace::SRGB).data);
    }

    // 刷新像素与深度缓冲
    void clearColor(const LinearColor& linear) noexcept {
      std::fill(color_buf_.begin(), color_buf_.end(), linear.toColor(ColorSpace::SRGB).data);
    }

    void clearDepth() noexcept {
      std::fill(depth_buf_.begin(), depth_buf_.end(), RenderConstants::DepthClearValue);
    }

    // 视区范围是 [0, width][0, height]，而屏幕坐标系是定义在左上角，+y 指向下的，故需翻转 y 轴
    // 缓冲区索引是从 0 开始的，故 y 值减一。
    [[nodiscard]] size_t getBufferIndex(size_t x, size_t y) const noexcept {
      return (h_ - y - 1) * w_ + x;
    }

    // 放置像素
    void setColor(u32 index, const LinearColor& linear) noexcept {
      if(pass_type_ == RenderPass::Depth) return;
      color_buf_[index] = linear.toneMapped().toColor(ColorSpace::SRGB).data;
    }

    // 深度测试
    [[nodiscard]] bool depthTest(u32 index, f32 new_dep) const noexcept {
      return new_dep > depth_buf_[index];
    }
    
    // 深度写入
    void writeDepth(u32 index, f32 new_dep) noexcept {
      depth_buf_[index] = new_dep;
    }

    [[nodiscard]] f32 sampleDepth(const Vec2f& uv) const noexcept {
      f32 u = std::clamp(uv.x(), 0.0f, 1.0f);
      f32 v = std::clamp(uv.y(), 0.0f, 1.0f);

      f32 px = u * static_cast<f32>(w_) - 0.5f;
      f32 py = v * static_cast<f32>(h_) - 0.5f;

      int x = std::clamp(static_cast<int>(px + 0.5f), 0, static_cast<int>(w_ - 1));
      int y = std::clamp(static_cast<int>(py + 0.5f), 0, static_cast<int>(h_ - 1));

      return depth_buf_[getBufferIndex(x, y)];
    }

    [[nodiscard]] u32* colorBuf() noexcept { return color_buf_.data(); }

    // 获取缓冲区宽/高
    [[nodiscard]] u32 width()  const noexcept { return w_; }
    [[nodiscard]] u32 height() const noexcept { return h_; }

  private:
    // 缓冲区宽高
    u32 w_;
    u32 h_;

    RenderPass pass_type_;

    // 像素与深度缓冲
    std::vector<u32> color_buf_;
    std::vector<f32> depth_buf_;
  };

} // namespace Medium