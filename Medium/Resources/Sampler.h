///
/// @file   Sampler.h
/// @author Garfield949
/// @date   2026/6/8 14:25
/// @brief
///
#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>

#include "Base/Types.h"
#include "Base/Color.h"
#include "Base/Math/Math.h"
#include "Resources/Image2D.h"
#include "Resources/Texture2D.h"

namespace Medium {

enum class FilterMode {
  Nearest,
  Linear,
  Trilinear  // 实时渲染中，三线性默认指代 mipmaps 采样。
};

enum class AddressMode {
  Repeat,
  Clamp
};

/// @class Sampler
class Sampler {
public:
  constexpr Sampler(FilterMode filter_mode, AddressMode address_mode_u, AddressMode address_mode_v)
    : filter_mode_{filter_mode}, address_mode_u_{address_mode_u}, address_mode_v_{address_mode_v} {}

  constexpr Sampler(FilterMode filter_mode, AddressMode address_mode)
    : Sampler(filter_mode, address_mode, address_mode) {}

  LinearColor sample(const Image2D<Color>& image, ColorSpace color_space, const Vec2f& uv) const {
    return filter_mode_ == FilterMode::Nearest ? sampleNearest(image, color_space, uv.x(), uv.y())
                                               : sampleBilinear(image, color_space, uv.x(), uv.y());
  }

  /// @param duv_dx: 屏幕像素 x 轴上移动一格 u,v 各自的增量。
  /// @param duv_dy: 屏幕像素 y 轴上移动一格 u,v 各自的增量。
  LinearColor sample(const Texture2D& tex, const Vec2f& uv, const Vec2f& duv_dx, const Vec2f& duv_dy) const {
    Vec2f size_base{tex.widthF(), tex.heightF()};

    Vec2f dtexel_dx = duv_dx.cwiseProduct(size_base);  // 屏幕像素 x 轴移动一格时，纹理在 u,v 方向上的纹素增量。
    Vec2f dtexel_dy = duv_dy.cwiseProduct(size_base);  // 屏幕像素 y 轴移动一格时，纹理在 u,v 方向上的纹素增量。

    // 计算 ρ^2.
    f32 texel_grad_squared = std::max(dtexel_dx.squaredNorm(), dtexel_dy.squaredNorm());

    // 当 ρ = 1，即 pixel 和 texel 一一对应时，log2(1) = 0，即选到原图 mip_level0.
    // 当 ρ < 1，变为放大过滤，可以想象当相机离墙很近时，一个纹素可能覆盖了很多个屏幕像素，此时直接用 level0.
    if(texel_grad_squared <= 1.0f)
      return sampleBilinear(tex.mipLevel(0), tex.colorSpace(), uv.x(), uv.y());

    // ρ = sqrt(texel_grad_squared) -> log2{(texel_grad_squared)^0.5f} = 0.5f * log2{texel_grad_squared}.
    // 当 ρ = 4，则 lod = log2(4) = 2. mip level 2 中一个像素对应原图的 16 个像素。
    f32 lod = 0.5f * std::log2(texel_grad_squared);

    i32 level_highest = tex.mipLevelCount() - 1;
    
    lod = std::min(lod, static_cast<f32>(level_highest));

    // 当 lod = 2.3, level_l = 2, level_h = 3.
    i32 level_l = static_cast<i32>(lod);
    i32 level_h = std::min(level_highest, level_l + 1);

    // 三次插值系数。
    f32 t = lod - static_cast<f32>(level_l);

    LinearColor color_l = sampleBilinear(tex.mipLevel(level_l), tex.colorSpace(), uv.x(), uv.y());

    if(level_l == level_h || t == 0.0f) return color_l;
    
    LinearColor color_h = sampleBilinear(tex.mipLevel(level_h), tex.colorSpace(), uv.x(), uv.y());
    
    return Math::lerp(color_l, color_h, t);
  }

private:
  LinearColor sampleNearest(const Image2D<Color>& image, ColorSpace color_space, f32 u, f32 v) const {
    i32 x = static_cast<i32>(std::floor(u * image.widthF()));
    i32 y = static_cast<i32>(std::floor(v * image.heightF()));

    x = addressCoord(x, image.width(),  address_mode_u_);
    y = addressCoord(y, image.height(), address_mode_v_);

    return image[x, y].toLinear(color_space);
  }

  LinearColor sampleBilinear(const Image2D<Color>& image, ColorSpace color_space, f32 u, f32 v) const {
    // - 物理意义：减去 0.5 后，纹理坐标取整可对齐到像素中心；
    // - 背景：纹素在网格中占据宽度为 1.0 的区域，特别地，最左下角的纹素占据 0 ~ 1 的坐标范围，易知其像素中心为 0.5; 然而纹理坐标
    //        取整后只能获得 0, 1, 2 这样的数值，此为纹素的边界；
    // - 原理：1. 比如 x = 4.2，则有 3.5 < 4.2 < 4.5，但 x 取整后要么是 4 要么是 5，无法取到像素中心；
    //        2. 减去 0.5 后，坐标轴整体往左平移 0.5 个单位，网格交界处正好对准像素中心。此时区间映射到 3.0 < 3.7 < 4.0，取整
    //           后正好对齐到像素中心；
    //        3. 双线性采样的定义正是取某纹理坐标周围四个纹素的中心，收益最大。
    f32 x = u * image.widthF()  - 0.5f;
    f32 y = v * image.heightF() - 0.5f;

    // 左下角像素坐标。
    i32 px = static_cast<i32>(std::floor(x));
    i32 py = static_cast<i32>(std::floor(y));

    i32 w_image = image.width();
    i32 h_image = image.height();

    // 计算 wrap 后的左下角和右上角像素坐标。
    i32 x0 = addressCoord(px    , w_image, address_mode_u_);
    i32 y0 = addressCoord(py    , h_image, address_mode_v_);
    i32 x1 = addressCoord(px + 1, w_image, address_mode_u_);
    i32 y1 = addressCoord(py + 1, h_image, address_mode_v_);

    // 获得四个角的像素颜色。
    LinearColor bl = image[x0, y0].toLinear(color_space), br = image[x1, y0].toLinear(color_space);
    LinearColor tl = image[x0, y1].toLinear(color_space), tr = image[x1, y1].toLinear(color_space);

    // 插值系数：距离未 wrap 前的左边界和下边界的偏移。
    f32 t1 = x - static_cast<f32>(px);
    f32 t2 = y - static_cast<f32>(py);

    // 水平混合。
    LinearColor bottom = Math::lerp(bl, br, t1);
    LinearColor top    = Math::lerp(tl, tr, t1);

    // 垂直混合。
    return Math::lerp(bottom, top, t2);
  }

  static i32 addressCoord(i32 coord, i32 size, AddressMode mode) {
    assert(size > 0);
    if(mode == AddressMode::Clamp)
      return std::clamp(coord, 0, size - 1);
    // 负数 coord 映射到 [0, size).
    i32 mod = coord % size;
    return mod < 0 ? mod + size : mod;
  }

  FilterMode filter_mode_     = FilterMode::Nearest;
  AddressMode address_mode_u_ = AddressMode::Repeat;
  AddressMode address_mode_v_ = AddressMode::Repeat;
};

} // namespace Medium

// 采样器作为全局实例。
namespace Medium::Samplers {

inline constexpr Sampler NearestRepeat{ FilterMode::Nearest, AddressMode::Repeat};
inline constexpr Sampler NearestClamp { FilterMode::Nearest, AddressMode::Clamp};

inline constexpr Sampler LinearClamp { FilterMode::Linear, AddressMode::Clamp};
inline constexpr Sampler LinearRepeat{ FilterMode::Linear, AddressMode::Repeat};

inline constexpr Sampler TrilinearClamp { FilterMode::Trilinear, AddressMode::Clamp};
inline constexpr Sampler TrilinearRepeat{ FilterMode::Trilinear, AddressMode::Repeat};

} // namespace Medium::Samplers
