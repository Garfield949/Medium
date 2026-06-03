///
/// @file   CSM.cpp
/// @author Garfield949
/// @date   2026/4/20 21:29
/// @brief  
///
#include "CSM.h"

#include <fmt/color.h>

#include "Base/Math/Math.h"

//   [ 透视畸变 Perspective Aliasing ]
// - 平行光的阴影贴图是均匀、精度有限的；
// - 透视投影的近大远小现象导致，屏幕空间内近处的像素密度远大于远处，即同一大小的世界空间区域，在屏幕空间近处和远处
//   所占的像素数不同；
// - 近处：屏幕采样率 >> 阴影贴图采样率，导致很多个像素争抢一个 texel;
//   远处：屏幕采样率 <= 阴影贴图采样率，几乎能实现一对一的映射关系，精度很高。
// - 重点：并不是远处不会走样，而是远处的像素变稀疏，走样没有暴露出来。
namespace Medium {

// - CSM 划分原则：近处的级联范围尽量划小一些，让单位面积的屏幕像素分到更多的精度；
// - 均匀划分：每个级联的 z 范围相同，未遵循划分原则；
// - 对数划分：目标是让切片范围形成等比关系 slices[i] / slices[i-1] = k.
//   - 推导：设 n = camera_near, f = max_shadow_dist, cnt = cascade_cnt. 根据假设，可得到等式
//                                  slices[i] = n * k^i.
//     已知条件为:
//                       当 i = cnt 时，slices[cnt] = n * k^cnt = f.
//     目标是求出 k. 移项得 k^cnt = f/n; 两边取对数有 cnt*ln(k) = ln(f/n); 移项得 ln(k) = ln(f/n)/cnt;
//     两边取 e 的底数即可求出:
//                                  k = (f/n)^(1/cnt).
//     将其代入前面的方程，可得 slices[i] = n * (f/n)^(i/cnt). 其中指数 i/cnt 即为循环里求的 t.
//   - 观察 slices[i] = n * k^i，即每个级联的范围相比上一级呈指数级增长（自变量在指数上），符合透视畸变的原理。
//   - 缺陷：当 n 过小时，划分的第一级联范围会过小。举例：n = 0.1, f = 1000, cnt = 4. 则有
//                 slices[1] - slices[0] = 0.1 * (1000 / 0.1)^0.25 = 1 - 0.1 = 0.9.
//     即划出的第一级联只跨越了不到一米的距离，几乎看不到，浪费大量精度；而后续级联宽度直接暴涨：9, 90, 900. 这
//     会导致不同级联分到的阴影精度剧烈突变，近处级联与远处级联的接缝会出现明显视觉跳变 -- 从清晰到非常模糊。
// - 混合划分：在近处加入均匀划分的贡献，平滑级联间的范围增长速度。还是按上面的例子：均匀划分的第一级联宽度为 250，
//   则有 slices[i] = 250 * (1 - 0.65) + 1.0 * 0.65 = 88. 解决了近处视锥体被极端压缩，精度浪费在镜头贴面上
//   的问题，并使各级联间的范围增长较为平滑；
// - cascade_cnt 已在场景读取阶段被钳制到 1 ~ 4.
std::array<f32, kMaxCSMCascades + 1> calcCSMSlices(f32 camera_near,
                                                   f32 max_shadow_dist,
                                                   u32 cascade_cnt)
{
  std::array<f32, kMaxCSMCascades + 1> slices{};

  slices[0] = camera_near;

  // lambda 决定均匀划分的贡献，取值 0 ~ 1. 越大均匀划分贡献越低，近处范围越小，精度越高。
  const f32 lambda = 0.65f;
  const f32 k = max_shadow_dist / camera_near;
  // cascade_cnt = 1 时循环不会执行，可正确处理。
  for(u32 i = 1; i < cascade_cnt; i++) {
    f32 t = static_cast<f32>(i) / static_cast<f32>(cascade_cnt);

    // 均匀划分
    f32 uniform_slice = Math::lerp(camera_near, max_shadow_dist, t);

    // 对数划分，不会发生除零，相机的 near 最低被钳制到 0.1.
    f32 log_slice = camera_near * std::pow(k, t);

    // 混合划分
    slices[i] = Math::lerp(uniform_slice, log_slice, lambda);
  }

  slices[cascade_cnt] = max_shadow_dist;

  fmt::print(fmt::emphasis::bold | fg(fmt::color::green), "\nCSM cascade slicing input:\n");
  fmt::print("Camera Near        : {}\n"
             "Max Shadow Distance: {}\n"
             "Cascade Count      : {}\n", camera_near, max_shadow_dist, cascade_cnt);

  fmt::print(fmt::emphasis::bold | fg(fmt::color::green), "\nCSM cascade slicing output:\n");
  for(u32 i = 0; i < cascade_cnt; i++) {
    fmt::print("Cascade {}: [{:.1f} ~ {:.1f}]\n", i, slices[i], slices[i+1]);
  }
  fmt::print("\n");

  return slices;
}

// CSM0, CSM1, CSM2, CSM3 依次占用 CSM Atlas 的左上角、右上角、左下角、右下角。
RectF calcCSMAtlasViewport(i32 cascade_index, u32 tile_res) {
  f32 tile_res_f = static_cast<f32>(tile_res);
  switch(cascade_index) {
  case 0:  return {0.0f      , tile_res_f, tile_res_f, tile_res_f}; // 左上 tile
  case 1:  return {tile_res_f, tile_res_f, tile_res_f, tile_res_f}; // 右上 tile
  case 2:  return {0.0f      , 0.0f      , tile_res_f, tile_res_f}; // 左下 tile
  default: return {tile_res_f, 0.0f      , tile_res_f, tile_res_f}; // 右下 tile
  }
}

} // namespace Medium