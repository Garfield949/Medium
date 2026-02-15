///
/// @file   Texture.cpp
/// @author Garfield949
/// @date   2026/2/14 11:42
/// @brief  
///
#include "Texture.h"

// 定义此宏以在该文件内展开 stb_image 的具体实现；
// 注意只能在一个文件内定义此宏（比如该文件内），以避免多重定义。
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <fmt/core.h>

namespace Medium {

    std::shared_ptr<Texture> Texture::loadFromFile(const std::filesystem::path& path) {
        // UV 空间的原点在左下角，而纹理图的存储是行优先的，原点在左上角。为了索引方便，先设置将读进的纹理上下翻转。
        stbi_set_flip_vertically_on_load(true);

        auto tex = std::shared_ptr<Texture>(new Texture());

        unsigned char* raw = stbi_load(path.string().c_str(),
                                            &tex->width_,
                                            &tex->height_,
                                         &tex->channels,
                                      STBI_rgb_alpha); // 强制补齐 alpha 通道。

        if(!raw) {
            fmt::print(stderr, "\nTexture::loadFromFile - StbImage::{}\n\n", stbi_failure_reason());
            return nullptr;
        }

        // 打印调试信息
        fmt::print("Texture::loadFromFile - [{}] - Success.\n"
                   "Height: {}\n"
                   "Width : {}\n\n", path.string(), tex->height_, tex->width_);

        tex->data_.reset(raw);

        return tex;
    }

    // 最近邻采样 Nearest Neighbor Sampling.
    Color Texture::sample(f32 U, f32 V) const {
        U -= std::floor(U); // Repeat 平铺模式，取小数部分：1.1 -> 0.1, 1.0 -> 0.0
        V -= std::floor(V);

        auto x = static_cast<int>(U * static_cast<f32>(width_)); // 0.999 * width 转型为 int 自动截断为 width - 1
        auto y = static_cast<int>(V * static_cast<f32>(height_));

        x = std::clamp(x, 0, width_ - 1);
        y = std::clamp(y, 0, height_ - 1);

        auto offset = (y * width_ + x) * 4; // R,G,B,A

        auto pixel = data_.get() + offset;

        return {pixel[0], pixel[1], pixel[2], pixel[3]};
    }

} // namespace Medium