///
/// @file   Sampler.h
/// @author Garfield949
/// @date   2026/6/8 14:25
/// @brief  
///
#pragma once

#include <algorithm>
#include <cassert>

#include "Base/Types.h"
#include "Base/Color.h"
#include "Base/Math/Math.h"
#include "Resources/Texture2D.h"

namespace Medium {

enum class FilterMode {
  Nearest,
  Linear
};

enum class AddressMode {
  Repeat,
  Clamp
};

class Sampler {
public:
  constexpr Sampler(FilterMode filter_mode, AddressMode address_mode_u, AddressMode address_mode_v)
    : filter_mode_{filter_mode}, address_mode_u_{address_mode_u}, address_mode_v_{address_mode_v} {}

  LinearColor sampleColor(const Texture2D& tex, f32 u, f32 v) const {    
    return filter_mode_ == FilterMode::Nearest ? sampleColorNearest(tex, u, v)
                                               : sampleColorBilinear(tex, u, v);
  }

private:
  LinearColor sampleColorNearest(const Texture2D& tex, f32 u, f32 v) const {
    i32 x = static_cast<i32>(std::floor(u * tex.widthF()));
    i32 y = static_cast<i32>(std::floor(v * tex.heightF()));

    x = interpretCoord(x, tex.width(),  address_mode_u_);
    y = interpretCoord(y, tex.height(), address_mode_v_);

    return tex.readColor(x, y);
  }

  LinearColor sampleColorBilinear(const Texture2D& tex, f32 u, f32 v) const {
    // - 物理意义：减去 0.5 后，纹理坐标取整可对齐到像素中心；
    // - 背景：纹素在网格中占据宽度为 1.0 的区域，特别地，最左下角的纹素占据 0 ~ 1 的坐标范围，易知其像素中心为 0.5; 然而纹理坐标
    //        取整后只能获得 0, 1, 2 这样的数值，此为纹素的边界；
    // - 原理：1. 比如 x = 4.2，则有 3.5 < 4.2 < 4.5，但 x 取整后要么是 4 要么是 5，无法取到像素中心；
    //        2. 减去 0.5 后，坐标轴整体往左平移 0.5 个单位，网格交界处正好对准像素中心。此时区间映射到 3.0 < 3.7 < 4.0，取整
    //           后正好对齐到像素中心；
    //        3. 双线性采样的定义正是取某纹理坐标周围四个纹素的中心，收益最大。
    f32 x = u * tex.widthF()  - 0.5f;
    f32 y = v * tex.heightF() - 0.5f;

    // 左下角像素坐标。
    i32 px = static_cast<i32>(std::floor(x));
    i32 py = static_cast<i32>(std::floor(y));

    // 计算 wrap 后的左下角和右上角像素坐标。
    i32 x0 = interpretCoord(px    , tex.width() , address_mode_u_);
    i32 y0 = interpretCoord(py    , tex.height(), address_mode_v_);
    i32 x1 = interpretCoord(px + 1, tex.width() , address_mode_u_);
    i32 y1 = interpretCoord(py + 1, tex.height(), address_mode_v_);

    // 获得四个角的像素颜色。
    LinearColor bl = tex.readColor(x0, y0), br = tex.readColor(x1, y0);
    LinearColor tl = tex.readColor(x0, y1), tr = tex.readColor(x1, y1);

    // 插值系数：距离未 wrap 前的左边界和下边界的偏移。
    f32 t1 = x - static_cast<f32>(px);
    f32 t2 = y - static_cast<f32>(py);
    
    // 水平混合。
    LinearColor bottom = Math::lerp(bl, br, t1);
    LinearColor top    = Math::lerp(tl, tr, t1);
    
    // 垂直混合。
    return Math::lerp(bottom, top, t2);
  }

  static i32 interpretCoord(i32 coord, u32 size, AddressMode mode) {
    assert(size > 0);
    i32 isize = static_cast<i32>(size);
    if(mode == AddressMode::Clamp)
      return std::clamp(coord, 0, isize - 1);
    // 负数 coord 映射到 [0, size).
    i32 mod = coord % isize;
    return mod < 0 ? mod + isize : mod;
  }

  FilterMode filter_mode_     = FilterMode::Nearest;
  AddressMode address_mode_u_ = AddressMode::Repeat;
  AddressMode address_mode_v_ = AddressMode::Repeat;
};

} // namespace Medium

// 采样器作为全局实例。
namespace Medium::Samplers {

inline constexpr Sampler NearestRepeat{
  FilterMode::Nearest,
  AddressMode::Repeat,
  AddressMode::Repeat
};

inline constexpr Sampler LinearRepeat{
  FilterMode::Linear,
  AddressMode::Repeat,
  AddressMode::Repeat
};

inline constexpr Sampler NearestClamp{
  FilterMode::Nearest,
  AddressMode::Clamp,
  AddressMode::Clamp
};

inline constexpr Sampler LinearClamp{
  FilterMode::Linear,
  AddressMode::Clamp,
  AddressMode::Clamp
};
  
} // namespace Medium::Samplers