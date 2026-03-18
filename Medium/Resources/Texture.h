///
/// @file   Texture.h
/// @author Garfield949
/// @date   2026/2/14 11:42
/// @brief  
///
#pragma once

#include "Base/Types.h"
#include "Base/Color.h"

namespace Medium {

  enum class FilterMode {
    Nearest, Bilinear
  };

  class Texture {
    friend class AssetCache;
  public:
    ~Texture() = default;

    [[nodiscard]] LinearColor sample(const Vec2f& UV, FilterMode filter_mode) const noexcept {
      // Repeat 平铺模式，取小数部分：1.1 -> 0.1, 1.0 -> 0.0; U, V 能取到的范围是 0 ~ 0.999...
      f32 u = UV.x() - std::floor(UV.x());
      f32 v = UV.y() - std::floor(UV.y());

      // 在材质两个轴上分别减 0.5，以将各像素的中心对齐到网格线交点
      // 比如 px = 4.2，则有 3.5 < 4.2 < 4.5. 减去 0.5 则区间重映射为 3.0 < 3.7 < 4.0
      f32 px = u * static_cast<f32>(width_)  - 0.5f;
      f32 py = v * static_cast<f32>(height_) - 0.5f;

      auto getPixel = [&](int x, int y) -> LinearColor {
        x = std::clamp(x, 0, width_  - 1);
        y = std::clamp(y, 0, height_ - 1);
        auto offset = (y * width_ + x) * 4; // 平铺的 R,G,B,A
        auto pixel = data_.get() + offset;
        return Color{pixel[0], pixel[1], pixel[2], pixel[3]}.toLinear(this->color_space_);
      };

      if(filter_mode == FilterMode::Nearest) {
        int x = static_cast<int>(px + 0.5f);
        int y = static_cast<int>(py + 0.5f);
        return getPixel(x, y);
      }

      // 左下角像素的坐标
      int x0 = static_cast<int>(std::floor(px));
      int y0 = static_cast<int>(std::floor(py));

      f32 t1 = px - static_cast<f32>(x0); // 距离左边界的偏移
      f32 t2 = py - static_cast<f32>(y0); // 距离下边界的偏移

      // 获得四个角的像素颜色
      LinearColor bl = getPixel(x0,     y0);
      LinearColor br = getPixel(x0 + 1, y0);
      LinearColor tl = getPixel(x0,     y0 + 1);
      LinearColor tr = getPixel(x0 + 1, y0 + 1);

      // 水平混合
      LinearColor bottom = bl + (br - bl) * t1;
      LinearColor top    = tl + (tr - tl) * t1;

      // 垂直混合
      return bottom + (top - bottom) * t2;
    }

    [[nodiscard]] int width()  const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }

  private:
    Texture() = default;

    struct StbImageDeleter { // 自定义删除器
      void operator()(unsigned char *data) const {
        if(data) free(data);
      }
    };

    ColorSpace color_space_ = ColorSpace::SRGB;

    int width_   = 0;
    int height_  = 0;
    int channels = 0;

    std::unique_ptr<unsigned char, StbImageDeleter> data_ = nullptr;
  };

} // Medium