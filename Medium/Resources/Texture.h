///
/// @file   Texture.h
/// @author Garfield949
/// @date   2026/2/14 11:42
/// @brief  
///
#pragma once

#include <filesystem>

#include "stb_image.h"

#include "Base/Types.h"
#include "Base/Forwards.h"
#include "Base/Color.h"

namespace Medium {

    class Texture {
    public:

        ~Texture() = default;

        static TexturePtr loadFromFile(const std::filesystem::path& path);

        static LinearColor getCheckerBoardColor(Vec2f UV) noexcept {
            // 定义棋盘格边长。
            f32 side = 8;
            // 将 [0.0 ~ 1.0] 的 UV 映射到 [0 ~ 8] 的棋盘格。
            int idxU = static_cast<int>(std::floor(UV.x() * side));
            int idxV = static_cast<int>(std::floor(UV.y() * side));
            // 奇偶校验
            if((idxU + idxV) % 2 == 0) return {0.5f}; 
            return {1.0f};
        }

        // 最近邻采样 Nearest Neighbor Sampling.
        [[nodiscard]] Color sample(Vec2f UV) const noexcept {
            // Repeat 平铺模式，取小数部分：1.1 -> 0.1, 1.0 -> 0.0; U, V 能取到的范围是 0 ~ 0.999...
            UV.x() -= std::floor(UV.x()); 
            UV.y() -= std::floor(UV.y());
    
            // 0.999 * width 转型为 int 自动截断为 width - 1
            auto x = static_cast<int>(UV.x() * static_cast<f32>(width_));
            auto y = static_cast<int>(UV.y() * static_cast<f32>(height_));
    
            x = std::clamp(x, 0, width_  - 1);
            y = std::clamp(y, 0, height_ - 1);
    
            // 平铺的 R,G,B,A
            auto offset = (y * width_ + x) * 4; 
    
            auto pixel = data_.get() + offset;
    
            return {pixel[0], pixel[1], pixel[2], pixel[3]};
        }

        [[nodiscard]] int width()  const noexcept { return width_; }
        [[nodiscard]] int height() const noexcept { return height_; }

    private:
        Texture() = default;

        struct StbImageDeleter { // 自定义删除器
            void operator()(unsigned char *data) const {
                stbi_image_free(data);
            }
        };

        int width_   = 0;
        int height_  = 0;
        int channels = 0;

        std::unique_ptr<unsigned char, StbImageDeleter> data_ = nullptr;
    };

} // Medium