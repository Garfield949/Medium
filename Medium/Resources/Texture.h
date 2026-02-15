///
/// @file   Texture.h
/// @author Garfield949
/// @date   2026/2/14 11:42
/// @brief  
///
#pragma once

#include <filesystem>

#include "stb_image.h"

#include "Core/Types.h"
#include "Core/Color.h"

namespace Medium {

    class Texture {
    public:
        ~Texture() = default;

        static std::shared_ptr<Texture> loadFromFile(const std::filesystem::path& path);

        [[nodiscard]] Color sample(f32 U, f32 V) const;

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