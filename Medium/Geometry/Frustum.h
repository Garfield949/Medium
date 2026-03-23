///
/// @file   Frustum.h
/// @author Garfield949
/// @date   2026/3/20 19:41
/// @brief  
///
#pragma once

#include "Geometry/Plane.h"
#include "Geometry/AABB.h"

namespace Medium {

  class Frustum {
  public:
    static Frustum fromMatrixVP(const Mat4f& mat_vp) noexcept {
      Vec4f row0 = mat_vp.row(0); Vec4f row1 = mat_vp.row(1);
      Vec4f row2 = mat_vp.row(2); Vec4f row3 = mat_vp.row(3);

      Frustum frustum;
      // 0: Left, 1: Right, 2: Top, 3: Bottom, 4: Near, 5: Far
      frustum.planes[0] = Plane{row3 + row0};
      frustum.planes[1] = Plane{row3 - row0};
      frustum.planes[2] = Plane{row3 - row1};
      frustum.planes[3] = Plane{row3 + row1};
      frustum.planes[4] = Plane{row3 - row2};
      frustum.planes[5] = Plane{row2};

      for(auto & plane : frustum.planes)
        plane.normalize();

      return frustum;
    }

    [[nodiscard]] bool intersects(const AABB3D& bbox) const noexcept {
      for(const auto & plane : planes) {
        const Vec3f& n = plane.normal();
        Vec3f p_vert = bbox.min;

        // 修正为在平面法向上距离最大的点
        if(n.x() >= 0) p_vert.x() = bbox.max.x();
        if(n.y() >= 0) p_vert.y() = bbox.max.y();
        if(n.z() >= 0) p_vert.z() = bbox.max.z();

        // 最有希望在平面内部的点都在外面，那整个 AABB 都在外面
        if(plane.signedDistTo(p_vert) < 0) return false;
      }
      return true;
    }

  private:
    Frustum() = default;
    std::array<Plane, 6> planes;
  };

} // namespace Medium
