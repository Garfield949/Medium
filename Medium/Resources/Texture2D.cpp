///
/// @file   Texture2D.cpp
/// @author Garfield949
/// @date   2026/9/6 17:08
/// @brief  
///
#include "Texture2D.h"

namespace Medium {

namespace {
/// @brief 降采样，将 Image 缩小为原来的一半。
/// @details 降采样的核心在于计算 dst 每个像素在 src 上占据的面积，并将面积内像素颜色的加权平均作为该像素的值。
/// @note 由于最终的加权平均必须在线性空间中完成，所以需传入图像颜色空间来决定如何 decode 像素颜色。
Image2D<Color> downsample(const Image2D<Color>& image_src, ColorSpace color_space) {
  // 8x8 -> 4x4; 5x3 -> 2x1; 1x4 -> 1x2.
  i32 w_dst = std::max(1, image_src.width()  / 2);
  i32 h_dst = std::max(1, image_src.height() / 2);

  // 计算 src 相比于 dst 在长宽上的倍率。
  f32 w_ratio = static_cast<f32>(image_src.width())  / static_cast<f32>(w_dst);
  f32 h_ratio = static_cast<f32>(image_src.height()) / static_cast<f32>(h_dst);
  
  Image2D<Color> image_dst(w_dst, h_dst);

  // 遍历 dst 的每个像素。
  for(i32 y = 0; y < h_dst; ++y) {
    // 假设 src 尺寸为 5x3, dst 尺寸为 2x1. 则 w_ratio = 2.5, h_ratio = 3.
    // 计算 dst 当前像素对应 src 在 y 轴上的跨度：
    // 假设 y = 1，对应 dst 在 y 轴上的第二个像素。那么该像素在 y 轴上占据 [0, 1], 对应 src 上的端点为 0 * 3 = 0 和 1 * 3 = 3.
    // 取整用到 floor 和 ceil: y0 = 2.6 时，其处于第三个像素 [2, 3] 之间，故向下取整去的第三个像素以将其包含；ceil 同理。
    f32 src_y0_f = h_ratio * static_cast<f32>(y);
    f32 src_y1_f = h_ratio * static_cast<f32>(y + 1);
    i32 src_y0_i = static_cast<i32>(std::floor(src_y0_f));
    i32 src_y1_i = static_cast<i32>(std::ceil(src_y1_f));

    for(i32 x = 0; x < w_dst; ++x) {
      // 计算 dst 当前像素对应 src 在 x 轴上的跨度：
      // 假设 x = 1，对应 dst 在 x 轴上的第二个像素。那么该像素在 x 轴上占据 [1, 2], 对应 src 上的端点为 1 * 2.5 = 2.5 和 2 * 2.5 = 5.
      f32 src_x0_f = w_ratio * static_cast<f32>(x);
      f32 src_x1_f = w_ratio * static_cast<f32>(x + 1);
      i32 src_x0_i = static_cast<i32>(std::floor(src_x0_f));
      i32 src_x1_i = static_cast<i32>(std::ceil(src_x1_f));

      // 默认构造 Alpha = 1.0. 作为累加器，这里用到灰度的构造函数并指定 Alpha = 0.0.
      LinearColor color_sum{0.0f, 0.0f};

      // 遍历 dst 当前像素在 src 里的矩形范围中的每个像素。
      for(i32 sy = src_y0_i; sy < src_y1_i; ++sy) {

        // 计算当前像素在 y 轴上的实际跨度。
        f32 wy = std::min(src_y1_f, static_cast<f32>(sy + 1)) - 
                 std::max(src_y0_f, static_cast<f32>(sy));
        
        for(i32 sx = src_x0_i; sx < src_x1_i; ++sx) {

          // 计算当前像素在 x 轴上的实际跨度。
          f32 wx = std::min(src_x1_f, static_cast<f32>(sx + 1)) - 
                   std::max(src_x0_f, static_cast<f32>(sx));

          // 计算当前像素的实际面积，易知 weight <= 1.0.
          f32 weight_norm = wx * wy;

          // 颜色累加必须先转换到线性空间。
          color_sum += image_src[sx, sy].toLinear(color_space) * weight_norm;
        }
      }

      // 计算 dst 当前像素对应 src 矩形范围的实际面积。
      f32 area = (src_x1_f - src_x0_f) * (src_y1_f - src_y0_f);

      // 计算颜色的加权平均值，并转换到原空间 Abgr8.
      image_dst[x, y] = (color_sum / area).toColor(color_space);
    }
  }

  return image_dst;
}

} // ===---------- 匿名命名空间，控制仅单文件可见 ----------===

Texture2D::Texture2D(Image2D<Color> base, ColorSpace space): color_space_{space} {
  mip_levels_.push_back(std::move(base));
}

void Texture2D::generateMips() {
  // 最高层 mip 一定是 1x1.
  while(mip_levels_.back().width() > 1 || mip_levels_.back().height() > 1)
    mip_levels_.push_back(downsample(mip_levels_.back(), color_space_));
}

} // namespace Medium