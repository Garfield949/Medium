///
/// @file   RotationMatrix.cpp
/// @author Garfield949
/// @date   2026/4/11 15:02
/// @brief  
///
#include "RotationMatrix.h"

#include "Base/Math/Math.h"

namespace Medium {

Mat3f RotationMatrix::MakeFromForward(const Vec3f& forward, const Vec3f& up_ref) {
  Mat3f rotation_world = Mat3f::Identity();

  if(forward.isZero()) return rotation_world;

  Vec3f F = forward.normalized();
  // 奇点二：测试 forward 与 up_ref 的重合程度，否则参考轴用世界 X 轴替代,这样计算出来的 right_world 会是世界 +Z 方
  // 向，视角会发生跳变，但至少保证不会崩溃；使用了 std::fabs，也囊括了垂直向下看的情况。
  Vec3f U = up_ref.isZero() ? Vec3f::UnitY() : up_ref.normalized();
  if(Math::isNearlyOne(std::fabs(U.dot(F))))
    U = Vec3f::UnitX(); // Fallback.

  Vec3f R = U.cross(F).normalized();
  Vec3f U_actual = F.cross(R);

  // 列填为对象的物体轴在世界空间的坐标（旋转）
  rotation_world.col(0) = R;
  rotation_world.col(1) = U_actual;
  rotation_world.col(2) = F;

  return rotation_world;
}

} // Medium