//
// nodiscard 为 C++17 引入的属性修饰符：当调用者未对返回值进行处理，编译器会发出一个警告
//
#pragma once

#include "Color.h"
#include "Math.h"
#include <iostream>

namespace Medium {
    enum class BufferType {
        Color, Depth
    };

    /// @class RenderTarget
    /// @brief 渲染对象类型
    /// @details 存储帧缓冲和深度缓冲
    class RenderTarget {
    public:
        // 构造函数
        explicit RenderTarget(u32 width = 800, u32 height = 600)
            : width_{width}, height_{height},
              frameBuf_(width * height, Color::Black().data),
              depthBuf_(width * height, INF_F32) {
        }

        // 刷新像素与深度缓冲
        void clearColor(Color color = Color::Black());
        void clearDepth();

        void clear(BufferType type);

        // 放置像素
        void setColor(u32 x, u32 y, Color color);

        // 返回像素缓冲的只读指针
        [[nodiscard]] const u32* frameBuf() { return frameBuf_.data(); }

        // 获取缓冲区宽/高
        [[nodiscard]] u32 width()  const noexcept { return width_;  }
        [[nodiscard]] u32 height() const noexcept { return height_; }

    private:
        // 缓冲区宽高
        u32 width_;
        u32 height_;

        // 像素与深度缓冲
        std::vector<u32> frameBuf_;
        std::vector<f32> depthBuf_;
    };

    // 实现 Framebuffer 成员函数 setColor
    inline void RenderTarget::setColor(u32 x, u32 y, Color color) {
        if(x < width_ && y < height_)
            frameBuf_[y * width_ + x] = color.data;
    }

    // 实现 Framebuffer 成员函数 clearColor
    inline void RenderTarget::clearColor(Color color) {
        std::fill(frameBuf_.begin(), frameBuf_.end(), color.data);
    }

    // 实现 Framebuffer 成员函数 clearDepth
    inline void RenderTarget::clearDepth() {
        std::fill(depthBuf_.begin(), depthBuf_.end(), INF_F32);
    }

} // namespace Medium