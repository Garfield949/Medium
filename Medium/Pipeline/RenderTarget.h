///
/// @file RenderTarget.h
/// @brief 渲染对象类型，管理各种缓冲
/// @details nodiscard 为 C++17 引入的属性修饰符：当调用者未对返回值进行处理，编译器会发出一个警告
///
#pragma once

#include "Base/Color.h"
#include "Pipeline/RenderConfig.h"

namespace Medium {

    /// @class RenderTarget
    /// @brief 渲染对象类型
    /// @details 存储帧缓冲和深度缓冲，vp 指代 Viewport
    class RenderTarget {
    public:
        // 构造函数
        explicit RenderTarget(u32 width = 800, u32 height = 600)
            : width_{width}, height_{height},
              frameBuf_(width * height, Color::Black().data),
              depthBuf_(width * height, RenderConfig::MEDIUM_DEPTH_CLEAR_VALUE) {
        }

        // 刷新像素与深度缓冲
        void clearColor(Color color = Color::Black()) noexcept {
            std::fill(frameBuf_.begin(), frameBuf_.end(), color.data);
        }

        void clearDepth() noexcept {
            std::fill(depthBuf_.begin(), depthBuf_.end(), RenderConfig::MEDIUM_DEPTH_CLEAR_VALUE);
        }

        // 视区范围是 [0, width][0, height]，而屏幕坐标系是定义在左上角，+y 指向下的，故需翻转 y 轴
        // 缓冲区索引是从 0 开始的，故 y 值减一。
        [[nodiscard]] size_t index(u32 x, u32 y) const noexcept {
            return (height_ - y - 1) * width_ + x;
        }

        // 放置像素
        void setColor(u32 x, u32 y, const Color& color) noexcept {
            frameBuf_[index(x, y)] = color.data;
        }

        // 深度测试
        [[nodiscard]] bool depthTest(u32 x, u32 y, f32 new_dep) const noexcept {
            return new_dep > depthBuf_[index(x, y)];
        }
        
        // 深度写入
        void writeDepth(u32 x, u32 y, f32 new_dep) noexcept {
            depthBuf_[index(x, y)] = new_dep;
        }
        
        // 返回像素缓冲的只读指针
        [[nodiscard]] u32* frameBuf() noexcept { return frameBuf_.data(); }

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
    }; // class RenderTarget

} // namespace Medium