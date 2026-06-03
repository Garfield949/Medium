///
/// @file   AABB3D.cpp
/// @author Garfield949
/// @date   2026/5/30 21:29
/// @brief  
///
#include "AABB3D.h"

namespace Medium {

// Arvo's Algorithm
AABB3D AABB3D::toWorldAABB(const Mat4f& mat_world) const noexcept {
  AABB3D bbox_world;
  bbox_world.min = bbox_world.max = mat_world.topRightCorner<3, 1>();

  for(i32 i = 0; i < 3; ++i) {
    for(i32 j = 0; j < 3; ++j) {
      f32 a = mat_world(i, j) * min(j);
      f32 b = mat_world(i, j) * max(j);
      if(a > b) {
        bbox_world.max(i) += a;
        bbox_world.min(i) += b;
      } else {
        bbox_world.max(i) += b;
        bbox_world.min(i) += a;
      }
    }
  }

  return bbox_world;
}

// 注意，std::array 是一种聚合体类型，底层维护一个原生数组。故返回值需套两层括号：
// 1) 第一层初始化 std::array 本身； 2) 第二层初始化里面的数组。
std::array<Vec3f, 8> AABB3D::corners() const noexcept {
  f32 min_x = min.x(), min_y = min.y(), min_z = min.z();
  f32 max_x = max.x(), max_y = max.y(), max_z = max.z();
  return std::array<Vec3f, 8> {{
    {min_x, max_y, min_z}, {max_x, max_y, min_z}, {min_x, min_y, min_z}, {max_x, min_y, min_z},
    {min_x, max_y, max_z}, {max_x, max_y, max_z}, {min_x, min_y, max_z}, {max_x, min_y, max_z}
  }};
}

} // namespace Medium