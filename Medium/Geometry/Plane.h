///
/// @file   Plane.h
/// @author Garfield949
/// @date   2026/3/20 19:42
/// @brief  
///
#pragma once

#include <utility>

#include "Base/Math/Math.h"
#include "Base/Types.h"

namespace Medium {

/// @class Plane
/// @brief      描述世界空间下的 3D 平面
/// @details    1. 平面方程 Ax + By + Cz = d 的几何意义：从原点 O 到平面上任意点 Q(x, y, z) 的向量 OQ 在平面法线
///                n = (A, B, C) 上的投影长度恒为 d. 其中 Ax + By + Cz 即 OQ 与 n 的点积；
///             2. 但更多用其一般形式: Ax + By + Cz - d = 0. 令 D = -d 得 Ax + By + Cz + D = 0，其中 D 即原点
///                到平面有符号距离的相反数；
///             3. 要算空间中任意点 P 到平面的距离，只需先算出 OP 在平面法向 n 上的投影长度，再减去原点到平面的距离 d
///                即可，也就是 dist = np + D.
class Plane {
public:
  Plane() = default;

  Plane(Vec3f normal, f32 D)
    : N{std::move(normal)}, D{D} {}

  Plane(const Vec4f& ABCD)
    : Plane(ABCD.head<3>(), ABCD.w()) {}

  void normalize() noexcept {
    if(f32 n_norm = N.norm(); !Math::isNearlyZero(n_norm)) {
      f32 norm_inv = 1.0f / n_norm;
      N *= norm_inv;
      D *= norm_inv;
    }
  }

  [[nodiscard]] f32 signedDistTo(const Vec3f& p) const noexcept { return N.dot(p) + D; }

  [[nodiscard]] const Vec3f& normal() const noexcept { return N; }

private:
  Vec3f N = Vec3f::Zero();
  f32   D = 0.0f;
};

} // namespace Medium
