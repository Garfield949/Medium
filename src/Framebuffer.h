//
// nodiscard 为 C++17 引入的属性修饰符：当调用者未对返回值进行处理，编译器会发出一个警告
//
#pragma once

#include "Color.h"
#include "Math.h"

namespace Medium {

    class Framebuffer {
    public:
        // 构造函数
        explicit Framebuffer(u32 width = 800, u32 height = 600)
            : width_(width), height_(height),
              pixelBuf_(width * height, Color::Black().argb),
              depthBuf_(width * height, INF_F32) {
        }

        // 刷新像素与深度缓冲
        // 修改一个即将被销毁的临时对象/右值是无意义的，所以引用加 const
        // TODO: 将像素和深度刷新合为一个函数
        void clearPixel(Color color = Color::Black());
        void clearDepth();

        // 放置像素
        void setPixel(u32 x, u32 y, Color color);

        // 计算屏幕索引，原点位于左上角
        [[nodiscard]] constexpr u32 index(u32 x, u32 y) const;

        // 返回像素缓冲的只读指针
        [[nodiscard]] const u32* pixelBuf() { return pixelBuf_.data(); }

        // 获取缓冲区宽/高
        [[nodiscard]] u32 width()  const noexcept { return width_;  }
        [[nodiscard]] u32 height() const noexcept { return height_; }

    private:
        // 缓冲区宽高
        u32 width_;
        u32 height_;

        // 像素与深度缓冲
        std::vector<u32> pixelBuf_;
        std::vector<f32> depthBuf_;
    };

    // 实现 Framebuffer 成员函数 index
    // 由于数组索引是从 0 开始的，所以有 y - 1
    constexpr u32 Framebuffer::index(u32 x, u32 y) const {
        assert(x < width_);
        assert(y < height_);
        return (height_ - y - 1) * width_ + x;
    }

    // 实现 Framebuffer 成员函数 setPixel
    inline void Framebuffer::setPixel(u32 x, u32 y, Color color) {
        pixelBuf_[index(x, y)] = color.argb;
    }

    // 实现 Framebuffer 成员函数 clearPixel
    inline void Framebuffer::clearPixel(Color color) {
        std::fill(pixelBuf_.begin(), pixelBuf_.end(), color.argb);
    }

    // 实现 Framebuffer 成员函数 clearDepth
    inline void Framebuffer::clearDepth() {
        std::fill(depthBuf_.begin(), depthBuf_.end(), INF_F32);
    }

} // namespace Medium