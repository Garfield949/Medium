///
/// @file   Texture2D.h
/// @author Garfield949
/// @date   2026/6/11 14:24
/// @brief  
///
#pragma once

#include "Base/Color.h"
#include "Image2D.h"

namespace Medium {

/// @class Texture2D
/// @brief 2D 纹理类，自动构造 mipmaps.
/// @note  Image2D 删除了拷贝构造和拷贝赋值，并声明了 default 移动构造和移动赋值。所以这些性质会隐式地传播
///        给 Texture2D.
class Texture2D {
public:
  /// @brief 接收一个基图，构造时自动生成 mip 链，需指定纹理的色彩空间。
  explicit Texture2D(Image2D<Color> base, ColorSpace space);

  i32 width()  const noexcept { return mip_levels_[0].width(); }
  i32 height() const noexcept { return mip_levels_[0].height(); }

  i32 widthF()  const noexcept { return mip_levels_[0].widthF(); }
  i32 heightF() const noexcept { return mip_levels_[0].heightF(); }

  i32 mipLevelCount() const noexcept {
    return static_cast<i32>(mip_levels_.size());
  }

  ColorSpace colorSpace() const noexcept { return color_space_; }

  const Image2D<Color>& mipLevel(i32 level) const noexcept {
    assert(level >= 0 && level < mipLevelCount());
    return mip_levels_[level];
  }

private:
  ColorSpace color_space_;
  std::vector<Image2D<Color>> mip_levels_;  // level 0 为全分辨率原图。
};

} // namespace Medium
