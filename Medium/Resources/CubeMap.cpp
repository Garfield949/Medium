///
/// @file   CubeMap.cpp
/// @author Garfield949
/// @date   2026/10/14 06:26
/// @brief  
///
#include "CubeMap.h"

#include "Base/Color.h"
#include "Sampler.h"

namespace Medium {

CubeMap CubeMap::FromHorizontalCross(const Image2D<Color>& image) {
  const std::array face_coords{
    Vec2i{2, 1}, Vec2i{0, 1},  // +-X
    Vec2i{1, 2}, Vec2i{1, 0},  // +-Y
    Vec2i{1, 1}, Vec2i{3, 1}   // +-Z
  };

  const i32 face_size = image.height() / 3;

  CubeMap res;

  for(i32 i = 0; i < 6; ++i) {
    auto& face = res.faces_[i];
    face.resize(face_size);

    const i32 origin_x = face_coords[i].x() * face_size;
    const i32 origin_y = face_coords[i].y() * face_size;

    for(i32 y = 0; y < face_size; ++y)
    for(i32 x = 0; x < face_size; ++x)
      face[x, y] = image[origin_x + x, origin_y + y];
  }

  return res;
}

LinearColor CubeMap::sample(const Vec3f& dir) const noexcept {
  const f32 x = dir.x(), x_abs = std::abs(x);
  const f32 y = dir.y(), y_abs = std::abs(y);
  const f32 z = dir.z(), z_abs = std::abs(z);

  // 最大分量决定视线先碰到 cube 的哪一面，dir 除以 max_component 得到 dir 与单位立方体 [-1,1]^3 表面交点。
  // - 比如 dir = {4, 2, 1}, 则 dir / 4 = {1, 0.5, 0.25}. 其余两个分量构成选定的 cube 面内横纵坐标 s,t.
  // - 无需归一化 dir: 假设 dir 模长 L, 则归一化向量为 {X/L, Y/L, Z/L}. 假设最大分量为 |Z|/L, 则其余两个分
  //   量为 (X/L)/(|Z|/L) = X/|Z| 和 (Y/L)/(|Z|/L) = Y/|Z|, 将 L 抵消掉了。
  const f32 max_component = std::max({x_abs, y_abs, z_abs});
  
  f32 s = 0.0f, t = 0.0f;

  CubeFace face_select;
  if(x_abs >= y_abs && x_abs >= z_abs) {
    face_select = x >= 0.0f ? CubeFace::PosX : CubeFace::NegX;
    s = (x >= 0.0f) ? -z : z;
    t = y;
  } else if(y_abs >= z_abs) {
    face_select = y >= 0.0f ? CubeFace::PosY : CubeFace::NegY;
    s = x;
    t = (y >= 0.0f) ? -z : z;
  } else {
    face_select = z >= 0.0f ? CubeFace::PosZ : CubeFace::NegZ;
    s = (z >= 0.0f) ? x : -x;
    t = y;
  }

  // s,t 映射 [-1,1] -> [0,1].
  Vec2f uv = 0.5f * (Vec2f{s / max_component, t / max_component} + Vec2f::Ones());

  // 寻址模式必须为 clamp, 否则会在边缘混合到对边的像素。
  return Samplers::LinearClamp.sample(
    faces_[static_cast<size_t>(face_select)], ColorSpace::SRGB, uv
  );
}

} // namespace Medium