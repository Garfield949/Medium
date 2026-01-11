//
// nodiscard 为 C++17 引入的属性修饰符：当调用者未对返回值进行处理，编译器会发出一个警告
//

#pragma once

#include "Color.h"

namespace Medium {

    class Framebuffer {
    public:
        Framebuffer(u32 width, u32 height);

        // 刷新像素与深度缓冲
        // 修改一个即将被销毁的临时对象/右值是无意义的，所以引用加 const
        void clearPixel(const Color &color = Color::Black());
        void clearDepth();

        // 放置像素
        void setPixel(u32 x, u32 y, const Color &color);

        // 计算屏幕索引，原点位于左上角
        [[nodiscard]]
        constexpr u32 index(u32 x, u32 y) const;

        // 获取缓冲区宽高
        [[nodiscard]]
        u32 width()  const { return width_;  }
        [[nodiscard]]
        u32 height() const { return height_; }

        const u32* pixelBuf() {
            return pixelBuf_.data();
        }

    private:
        // 缓冲区宽高
        u32 width_;
        u32 height_;

        // 像素与深度缓冲
        std::vector<u32> pixelBuf_;
        std::vector<f32> depthBuf_;
    };

    // Framebuffer 成员函数 index 的实现
    // 由于数组索引是从 0 开始的，所以有 y - 1
    constexpr u32 Framebuffer::index(u32 x, u32 y) const {
        assert(x < width_);
        assert(y < height_);
        return (height_ - y - 1) * width_ + x;
    }

} // namespace Medium