///
/// @file   RotationMatrix.h
/// @author Garfield949
/// @date   2026/4/11 15:02
/// @brief  
///
#pragma once

#include "Base/Types.h"

namespace Medium {

/// @class RotationMatrix
/// @brief 根据条件构造纯旋转矩阵
class RotationMatrix {
public:
  /// @brief 根据传入的 forward 向量构造一个纯旋转矩阵，默认参考 up 向量为世界 +y
  static Mat3f MakeFromForward(const Vec3f& forward, const Vec3f& up_ref = Vec3f::UnitY());
};

} // Medium
