///
/// @file   DirectionalViewFit.cpp
/// @author Garfield949
/// @date   2026/4/9 12:23
/// @brief  
///
#include "DirectionalViewFit.h"

#include "Base/Math/RotationMatrix.h"
#include "Scene/Light/DirectionalLight.h"

namespace {

using namespace Medium;
// 注意：直觉上是，将场景AABB八个角点变换到光空间后，比较处于已定义 ortho 范围内的角点 z 的最大和最小值，从而计算出
// near 和 far. 但问题就出在，场景AABB的角点有很大的几率超出了 ortho 的范围，可能会导致求出的 near 和 far 太松。
// 此时必须求场景AABB与光视椎四个侧面的交集，再从交集中取得 z 的最大最小值；
// 流程：将场景AABB的六个面拆为12个三角形；对每个三角形，用光视椎的四个侧面进行裁剪（即 x 和 y 轴的范围）。裁剪后的
// 三角形集合即为求场景AABB在光视椎中的部分。
std::pair<f32, f32> calcNearAndFar(const Vec2f& light_ortho_min, 
                                   const Vec2f& light_ortho_max,
                                   const std::array<Vec3f, 8>& scene_bbox_corners_ls) {

}

DirectionalViewFit tightFit(const DirectionalViewFitDesc& fit_desc) {
  // ---------- 计算方向光的 world matrix（仅含旋转），并获得光空间 view matrix
  Mat3f mat_light_rot_world = RotationMatrix::makeFromForward(fit_desc.light_dir);
  Mat3f mat_view_rot_ls = mat_light_rot_world.transpose();

  // ---------- 获取主相机视椎在光空间下角点的包围盒
  auto cam_frustum_corners_ws
    = fit_desc.camera->slicedFrustumCornersWorld(fit_desc.slice_far);
  std::array<Vec3f, 8> cam_frustum_corners_ls;
  for(int i = 0; i < 8; i++)
    cam_frustum_corners_ls[i] = mat_view_rot_ls * cam_frustum_corners_ws[i];

  AABB3D bbox_cam_frustum_ls;
  for(const auto& v: cam_frustum_corners_ls)
    bbox_cam_frustum_ls.addPoint(v);

  // ---------- Texel Snapping
  // 计算阴影贴图一个纹素占据多少个世界空间单位长度。选用相机视椎对角线长度为分子，比较稳定 (0 为 near 的左上角点，7 为 far 的右下角点)。
  f32 cam_frustum_diagonal
    = (cam_frustum_corners_ws[0] - cam_frustum_corners_ws[7]).norm();

  f32 world_units_per_texel
    = cam_frustum_diagonal / static_cast<f32>(fit_desc.shadow_res);

  // 取得包围盒在光空间 xy 轴上的最小/大角点，并吸附到 texel 大小的整数倍。
  Vec2f light_ortho_min_snapped = (bbox_cam_frustum_ls.min.head<2>() / world_units_per_texel)
                                  .array()
                                  .floor() * world_units_per_texel;

  Vec2f light_ortho_max_snapped = (bbox_cam_frustum_ls.max.head<2>() / world_units_per_texel)
                                  .array()
                                  .floor() * world_units_per_texel;

  // ---------- 计算场景AABB在光空间下的角点
  auto scene_bbox_corners_ws = fit_desc.bbox_scene_ws.corners();
  std::array<Vec3f, 8> scene_bbox_corners_ls;
  for(int i = 0; i < 8; i++)
    scene_bbox_corners_ls[i] = mat_view_rot_ls * scene_bbox_corners_ws[i];

  // ---------- 求光视椎侧面与场景AABB的交集，取交集的 Zmin 和 Zmax 作为 near 和 far
  auto [near, far]
    = calcNearAndFar(light_ortho_min_snapped, light_ortho_max_snapped, scene_bbox_corners_ls);

  // 取相机视椎包围盒的 xy 中心作为方向光的位置
  Vec3f dir_light_pos_ls =  {
    (light_ortho_min_snapped.x() + light_ortho_max_snapped.x()) * 0.5f,
    (light_ortho_min_snapped.y() + light_ortho_max_snapped.y()) * 0.5f,
    near
  };

  // 注意，根据光相机的逻辑，near 永远为 0，所以 DirectionalViewFit 中的 near 设为 0，而 far 应设为 far - near，
  // 同时将光源位置的 z 设为 near (注意还得变换到世界空间中)；
  return {
    .light_pos_ws = mat_light_rot_world * dir_light_pos_ls,
    .ortho_near   = 0.0f,
    .ortho_far    = far - near,
    .ortho_width  = light_ortho_max_snapped.x() - light_ortho_min_snapped.x(),
    .ortho_height = light_ortho_max_snapped.y() - light_ortho_min_snapped.y(),
    .shadow_texel_size_ws = world_units_per_texel
  };
}

DirectionalViewFit stableFit(const DirectionalViewFitDesc& fit_desc) {
  // 获取主相机视椎切片的外接球
  Sphere cam_circum_sphere_ws = fit_desc.camera->slicedFrustumCircumSphereWorld(fit_desc.slice_far);

  // 外接球直径作为光源视椎的长宽尺寸
  f32 diameter = cam_circum_sphere_ws.radius * 2;

  // Texel Snapping: 计算阴影贴图一个纹素占据多少个世界空间单位长度
  f32 world_units_per_texel = diameter / static_cast<f32>(fit_desc.shadow_res);

  // 方向光的 world matrix
  Mat3f mat_light_world_rot = RotationMatrix::makeFromForward(fit_desc.light_dir);
  // view matrix
  Mat3f mat_view_rot_ls = mat_light_world_rot.transpose();
  Vec3f shpere_center_snapped_ls = mat_view_rot_ls * cam_circum_sphere_ws.center;

  // 将坐标对齐为 texel_size 的整数倍
  shpere_center_snapped_ls.x() = std::round(shpere_center_snapped_ls.x() / world_units_per_texel) *
      world_units_per_texel;
  shpere_center_snapped_ls.y() = std::round(shpere_center_snapped_ls.y() / world_units_per_texel) *
      world_units_per_texel;

  // 球心转回世界空间，ws 代表 world space
  Vec3f sphere_center_snapped_ws = mat_light_world_rot * shpere_center_snapped_ls;

  // 构建光源位置：从球心开始，往光源 dir 的反向进行 球半径 的位移.
  Vec3f light_pos = sphere_center_snapped_ws - fit_desc.light_dir * (cam_circum_sphere_ws.radius);

  return {
    .light_pos_ws         = light_pos,
    .ortho_near           = 0.0f,
    .ortho_far            = diameter,
    .ortho_width          = diameter,
    .ortho_height         = diameter,
    .shadow_texel_size_ws = world_units_per_texel
  };
}

} // 匿名命名空间，控制仅单文件可见


namespace Medium {

DirectionalViewFit calcDirectionalViewFit(const DirectionalViewFitDesc& fit_desc,
                                          DirectionalFitMode mode) {
  return mode == DirectionalFitMode::Stable ? stableFit(fit_desc) : tightFit(fit_desc);
}

} // namespace Medium

