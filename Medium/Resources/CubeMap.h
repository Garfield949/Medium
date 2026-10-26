///
/// @file   CubeMap.h
/// @author Garfield949
/// @date   2026/10/14 06:26
/// @brief  
///
#pragma once

#include <memory>
#include <utility>

#include "Base/Color.h"
#include "Base/Types.h"
#include "Resources/Image2D.h"

namespace Medium {

enum class CubeFace: size_t {
  PosX = 0, NegX, PosY, NegY, PosZ, NegZ
};

/// @class CubeMap
/// @brief 立方体纹理类型。
class CubeMap {
public:
  explicit CubeMap() = default;

  /// @brief 读入原始水平十字 Image，拆分六个面并构造为一个 CubeMap 返回。
  static CubeMap FromHorizontalCross(const Image2D<Color>& image);

  /// @brief 根据输入视线方向读取对应 cube 面颜色。
  LinearColor sample(const Vec3f& direction) const noexcept;

  i32 resolution() const noexcept { return faces_[0].width(); }

  const Image2D<Color>& face(CubeFace face) const noexcept { return faces_[std::to_underlying(face)]; }

private:
  // 存储顺序：+X, -X, +Y, -Y, +Z, -Z.
  std::array<Image2D<Color>, 6> faces_{};
};

} // namespace Medium
