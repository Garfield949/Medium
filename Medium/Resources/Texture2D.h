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
  /// @brief 接收一个基图，需指定色彩空间。
  explicit Texture2D(Image2D<Color> base_image, ColorSpace color_space);

  i32 width()   const noexcept { return mip_levels_[0].width();   }
  i32 height()  const noexcept { return mip_levels_[0].height();  }
  f32 widthF()  const noexcept { return mip_levels_[0].widthF();  }
  f32 heightF() const noexcept { return mip_levels_[0].heightF(); }

  /// @brief 返回 mip 链层级数。
  i32 mipCount() const noexcept {
    return static_cast<i32>(mip_levels_.size());
  }

  /// @brief 返回纹理的原始色彩空间。
  ColorSpace colorSpace() const noexcept { return color_space_; }

  /// @brief 生成 Mipmap 链。
  void generateMips();

  /// @brief 获取指定 mip 层级。
  /// @note  std::vector::operator[] 返回的是引用，因此 decltype(auto) 一定会返回引用。
  const Image2D<Color>& mipLevel(i32 level) const noexcept {
    assert(level >= 0 && level < mipCount());
    return mip_levels_[level];
  }

  /// @brief 获取原图
  decltype(auto) baseImage(this auto& self) noexcept {
    return self.mip_levels_[0];
  }

private:
  std::vector<Image2D<Color>> mip_levels_;  // level 0 为全分辨率原图。
  ColorSpace color_space_;
};

} // namespace Medium
