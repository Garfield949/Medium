///
/// @file   Texture.h
/// @author Garfield949
/// @date   2026/2/14 11:42
/// @brief  
///
#pragma once

#include "Base/Math/Math.h"
#include "Base/Color.h"

namespace Medium {

  /// @class Texture
  /// @brief 存放实际材质数据，并负责材质采样
  /// @details 注意直接存 stbi 的指针即可，没必要存一个 LinearColor 然后在 AssetCache 装配
  ///          阶段提前将读入的材质进行 degamma. 原因：原始数据格式中，每像素占 4 字节 (4 个)
  ///          unsigned char; 容器换成 std::vector<LinearColor> 的话，每像素占 16 字节，
  ///          即 4 个 float，占用空间是前者的四倍，会造成频繁的 cache miss 反而使性能下降。
  class Texture {
    friend class AssetCache;
  public:
    enum class FilterMode { Nearest, Bilinear };
    
    [[nodiscard]] LinearColor texel(i32 x, i32 y) const noexcept {
      x = std::clamp(x, 0, w_ - 1);
      y = std::clamp(y, 0, h_ - 1);
      // 平铺的 R,G,B,A
      auto offset = (y * w_ + x) * 4; 
      auto pixel = data_.get() + offset;
      
      LinearColor res = Color{pixel[0], pixel[1], pixel[2], pixel[3]}.toLinear(color_space_);
      return res;
    }

    [[nodiscard]] LinearColor sample(const Vec2f& uv, FilterMode filter_mode) const noexcept {
      // - Repeat 平铺模式，取小数部分：1.1 -> 0.1, 1.0 -> 0.0
      // - U, V 能取到的范围是 0 ~ 0.9999..
      f32 U = uv.x() - std::floor(uv.x());
      f32 V = uv.y() - std::floor(uv.y());

      // - 物理意义：减去 0.5 后，纹理坐标取整可对齐到像素中心；
      // - 背景：纹素在网格中占据宽度为 1.0 的区域，特别地，最左下角的纹素占据 0 ~ 1 的坐标范围，易知其像素中心为 0.5; 然而纹理坐标
      //        取整后只能获得 0, 1, 2 这样的数值，此为纹素的边界；
      // - 原理：1. 比如 x = 4.2，则有 3.5 < 4.2 < 4.5，但 x 取整后要么是 4 要么是 5，无法取到像素中心；
      //        2. 减去 0.5 后，坐标轴整体往左平移 0.5 个单位，网格交界处正好对准像素中心。此时区间映射到 3.0 < 3.7 < 4.0，取整
      //           后正好对齐到像素中心；
      //        3. 双线性采样的定义正是取某纹理坐标周围四个纹素的中心，收益最大。
      // - 注意：当 U 或 V 为 0，则 x_tmp 或 y_tmp 会变成 -0.5，进而取整得到 -1; 而当 U 或 V 很接近 1 时，它俩又会很接近 w_-1
      //   或 h_-1，进行双线性采样时，会取到 w_ 或 h_ 造成越界
      f32 x_tmp = U * static_cast<f32>(w_) - 0.5f;
      f32 y_tmp = V * static_cast<f32>(h_) - 0.5f;

      if(filter_mode == FilterMode::Nearest) {
        i32 x = static_cast<i32>(std::lround(x_tmp));
        i32 y = static_cast<i32>(std::lround(y_tmp));
        // 注意，取整可能会越界，应做 wrap 处理，但开销较大，故仅在 texel 里做 clamp 处理；
        // 所以，该采样函数的行为是，99% 情况能正确处理平铺，但纹理接缝处会变成 clamp.
        return texel(x, y);
      }

      // 左下角像素的坐标
      i32 x0 = static_cast<i32>(std::floor(x_tmp));
      i32 y0 = static_cast<i32>(std::floor(y_tmp));

      f32 t1 = x_tmp - static_cast<f32>(x0); // 距离左边界的偏移
      f32 t2 = y_tmp - static_cast<f32>(y0); // 距离下边界的偏移

      // 获得四个角的像素颜色
      LinearColor bl = texel(x0, y0), br = texel(x0 + 1, y0);
      LinearColor tl = texel(x0, y0 + 1), tr = texel(x0 + 1, y0 + 1);
      // 水平混合
      LinearColor bottom = Math::lerp(bl, br, t1), top = Math::lerp(tl, tr, t1);
      // 垂直混合
      return Math::lerp(bottom, top, t2);
    }

    [[nodiscard]] u32 width()  const noexcept { return w_; }
    [[nodiscard]] u32 height() const noexcept { return h_; }

  private:
    Texture() = default;

    i32 w_ = 0;
    i32 h_ = 0;
    ColorSpace color_space_ = ColorSpace::SRGB;

    // 自定义仿函数删除器
    struct StbImageDeleter {
      void operator()(unsigned char *data) const {
        if(data) free(data);
      }
    };

    std::unique_ptr<unsigned char, StbImageDeleter> data_ = nullptr;
  };

} // Medium