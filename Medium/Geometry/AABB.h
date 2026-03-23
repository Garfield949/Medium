///
/// @file   AABB.h
/// @author Garfield949
/// @date   2026/3/20 16:36
/// @brief  
///
#pragma once

#include "Base/Types.h"
#include "Geometry/Subpixel.h"

namespace Medium {

  struct AABB3D {
    AABB3D() {
      reset();
    }
    
    [[nodiscard]] constexpr bool valid() const noexcept {
      return min.x() <= max.x() && min.y() <= max.y() && min.z() <= max.z();
    }

    void reset() noexcept {
      min.fill(std::numeric_limits<f32>::max());
      max.fill(std::numeric_limits<f32>::lowest());
    }

    // cwise 系列是 Eigen 的逐系数 coefficient-wise 方法
    void merge(const Vec3f& p) {
      min = min.cwiseMin(p);
      max = max.cwiseMax(p);
    }

    [[nodiscard]] Vec3f center() const noexcept {
      return (min + max) / 2.0f;
    }
    
    // Arvo's Algorithm
    [[nodiscard]] AABB3D toWorldAABB(const Mat4f& mat_wld) const noexcept {
      AABB3D bbox_wld;
      bbox_wld.min = bbox_wld.max = mat_wld.topRightCorner<3, 1>();

      for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < 3; ++j) {
          f32 a = mat_wld(i, j) * min(j);
          f32 b = mat_wld(i, j) * max(j);
          if(a > b) {
            bbox_wld.max(i) += a;
            bbox_wld.min(i) += b;
          } else {
            bbox_wld.max(i) += b;
            bbox_wld.min(i) += a;
          }
        }
      }
      return bbox_wld;
    }

    Vec3f min;
    Vec3f max;
  };

  struct ScreenAABB {
    ScreenAABB() = default;

    /// @brief 判断包围盒是否有效
    [[nodiscard]] constexpr bool valid() const noexcept {
      return min.x() <= max.x() && min.y() <= max.y();
    }

    static ScreenAABB fromSubpixelTri(const std::array<Vec2i, 3>& tri_subpixel,
                                                              u32 width_screen,
                                                              u32 height_screen) {
      ScreenAABB bbox;

      i32 x_min = std::min({tri_subpixel[0].x(), tri_subpixel[1].x(), tri_subpixel[2].x()});
      i32 x_max = std::max({tri_subpixel[0].x(), tri_subpixel[1].x(), tri_subpixel[2].x()});
      i32 y_min = std::min({tri_subpixel[0].y(), tri_subpixel[1].y(), tri_subpixel[2].y()});
      i32 y_max = std::max({tri_subpixel[0].y(), tri_subpixel[1].y(), tri_subpixel[2].y()});

      // 包围盒范围从 Subpixel 转到 Pixel 空间。进行安全的取整策略：宁可多框一些像素，避免漏掉边缘像素；
      // 1. Floor   行为：单纯右移。使得在 [160, 176) 范围内取整结果为 10；
      // 2. Ceiling 行为：加一个偏置再右移。这里加 15，使得在 (160, 176] 范围内取整结果为 11；
      bbox.min.x() = std::max(0, x_min >> Subpixel::Bits);
      bbox.min.y() = std::max(0, y_min >> Subpixel::Bits);
      bbox.max.x() = std::min(static_cast<int>(width_screen  - 1), (x_max + Subpixel::Mask) >> Subpixel::Bits);
      bbox.max.y() = std::min(static_cast<int>(height_screen - 1), (y_max + Subpixel::Mask) >> Subpixel::Bits);

      return bbox;
    }

    // 使用 int 是因为非法 BBOX 可能会有负值
    Vec2i min; // 左下角
    Vec2i max; // 右上角
  };

} // namespace Medium