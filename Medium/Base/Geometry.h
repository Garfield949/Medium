///
/// @file Geometry.h
/// @brief 放置通用的几何、计算相关的类型与函数
///
#pragma once

#include "Base/Subpixel.h"
#include "Math.h"

namespace Medium::Geometry {

  /// @struct BBox2D
  /// @brief 屏幕空间包围盒类型
  struct BBox2D {
    // 使用 int 是因为非法 BBOX 可能会有负值
    Vec2i min; // 左下角
    Vec2i max; // 右上角

    BBox2D() = default;

    /// @brief BBox2D 成员函数，判断包围盒是否有效
    [[nodiscard]] constexpr bool valid() const noexcept {
      return min.x() <= max.x() && min.y() <= max.y();
    }
    
    static BBox2D fromSubpixelTri(const std::array<Vec2i, 3>& tri_subpixel, u32 width_viewport, u32 height_viewport) {
      BBox2D bbox;
      
      i32 x_min = std::min({tri_subpixel[0].x(), tri_subpixel[1].x(), tri_subpixel[2].x()});
      i32 x_max = std::max({tri_subpixel[0].x(), tri_subpixel[1].x(), tri_subpixel[2].x()});
      i32 y_min = std::min({tri_subpixel[0].y(), tri_subpixel[1].y(), tri_subpixel[2].y()});
      i32 y_max = std::max({tri_subpixel[0].y(), tri_subpixel[1].y(), tri_subpixel[2].y()});
      
      // 包围盒范围从 Subpixel 转到 Pixel 空间。进行安全的取整策略：宁可多框一些像素，避免漏掉边缘像素；
      // 1. Floor   行为：单纯右移。使得在 [160, 176) 范围内取整结果为 10；
      // 2. Ceiling 行为：加一个偏置再右移。这里加 15，使得在 (160, 176] 范围内取整结果为 11；
      bbox.min.x() = std::max(0, x_min >> Subpixel::Bits);
      bbox.min.y() = std::max(0, y_min >> Subpixel::Bits);
      bbox.max.x() = std::min(static_cast<int>(width_viewport  - 1), (x_max + Subpixel::Mask) >> Subpixel::Bits);
      bbox.max.y() = std::min(static_cast<int>(height_viewport - 1), (y_max + Subpixel::Mask) >> Subpixel::Bits);
      
      return bbox;
    }
  }; // struct BBox2D

} // namespace Medium

namespace Medium::Geometry {

  /// @brief 浮点向量叉积；
  /// @param v1   向量 1;
  /// @param v2   向量 2;
  /// @details    1. 计算 v1 x v2. 若 v2 在 v1 的 “右边（顺时针）”，左手系下四指从 v1 弯曲到 v2，则大拇指朝外；
  ///                而在左手系中，大拇指朝屏幕外说明结果是负值；
  ///             2. 2D 叉积其实就是 3D 叉积 (x1,y1,0) 与 (x2,y2,0) 叉积所得向量 (0,0,x1y2-x2y1) 的 z 值。
  inline f32 cross2D(const Vec2f& v1, const Vec2f& v2) noexcept {
    return v1.x() * v2.y() - v2.x() * v1.y();
  }

  /// @brief 整型向量叉积；
  /// @details    两个 i32 相乘有溢出的风险，返回 i64 更安全。
  inline i64 cross2D(const Vec2i& v1, const Vec2i& v2) noexcept {
    return static_cast<i64>(v1.x()) * v2.y() - static_cast<i64>(v2.x()) * v1.y();
  }

  /// @brief 接收三角面片的三个顶点，计算其在 2D 空间内的朝向；
  /// @details    计算向量 (b-a) 与 (c-a) 的叉积；
  /// @return     > 0 为 CCW，< 0 为 CW，=0 为共线面积为 0.
  inline f32 orient2D(const Vec4f& a, const Vec4f& b, const Vec4f& c) noexcept {
    Vec2f v1 = b.head<2>() - a.head<2>();
    Vec2f v2 = c.head<2>() - a.head<2>();
    return cross2D(v1, v2);
  }

  /// @brief 边函数计算
  inline i64 edgeEquation(const Vec2i& edge_start, const Vec2i& edge_end, const Vec2i& test_point) {
    Vec2i V = test_point - edge_start;
    Vec2i E = edge_end   - edge_start;
    return cross2D(V, E);
  }
  
  /// @brief Top-Left Rule 的偏置计算。
  /// @param dx    待测试向量的两端 x 增量；
  /// @param dy    待测试向量的两端 y 增量；
  /// @details     1. 若像素落在两个三角形共享的边上，则让拥有 Top-Left 边的三角形对其着色；
  ///              2. 顶边的判定：其 y 增量为 0（水平），且方向向右（左手系，顺时针）；
  ///                 左边的判定：其 y 增量为正。
  /// @return 正好落在三角形边上的屏幕坐标的边函数为零，在增量更新的里层判断会通过并被着色。所以判断三条边
  ///         哪一个不是 Top-Left 型的，返回一个负偏置使该坐标对应边的 Equation 为负，进而无法着色。
  inline i32 topLeftBias(i32 dx, i32 dy) noexcept {
    bool is_top_left = (dy == 0 && dx > 0) || dy > 0;
    return is_top_left ? 0 : -1;
  };

} // namespace Medium
