///
/// @file   DirectionalViewFit.cpp
/// @author Garfield949
/// @date   2026/4/9 12:23
/// @brief  
///
#include "DirectionalViewFit.h"

#include "Base/Math/RotationMatrix.h"
#include "Scene/Light/DirectionalLight.h"
#include "Scene/Camera.h"
#include "DirectionalViewClip.h"

namespace {

using namespace Medium;

// 场景包围盒三角形索引表
constexpr size_t kBBoxTriNum = 12;
constexpr std::array<u8, kBBoxTriNum * 3> kSceneBBoxTriIndices {
  0, 1, 2,  2, 1, 3, // 前
  6, 4, 2,  4, 0, 2, // 左
  3, 1, 5,  3, 5, 7, // 右
  0, 4, 5,  5, 1, 0, // 上
  6, 2, 3,  6, 3, 7, // 下
  7, 4, 6,  5, 4, 7, // 后
};

std::pair<f32, f32> calcNearAndFar(const Vec2f& light_ortho_min,
                                   const Vec2f& light_ortho_max,
                                   const std::array<Vec3f, 8>& scene_bbox_corners_lb) {
  // 近远平面分别初始化为最大正数和最小负数。
  f32 near = std::numeric_limits<f32>::max();
  f32 far  = std::numeric_limits<f32>::lowest();

  // 遍历场景包围盒的三角形
  for(size_t i = 0; i < kBBoxTriNum; i++) {
    ClipPolygon clip_poly;

    for(size_t j = 0; j < 3; ++j, ++clip_poly.num_verts)
      clip_poly.verts[j] = scene_bbox_corners_lb[kSceneBBoxTriIndices[(i * 3) + j]];

    if(!clip(light_ortho_min, light_ortho_max, clip_poly)) continue;
    
    for(size_t k = 0; k < clip_poly.num_verts; k++) {
      if(clip_poly.verts[k].z() < near) near = clip_poly.verts[k].z();
      if(clip_poly.verts[k].z() > far)  far  = clip_poly.verts[k].z();
    }
  }

  return { near, far };
} // calcNearAndFar

} // 匿名命名空间，控制仅单文件可见


namespace Medium {

DirectionalViewFit calcDirectionalViewFit(const DirectionalViewFitParams& params) {
  // 结构化绑定，auto 推导为 DirectionalViewFitParams. Camera 本身是一个 const Camera*, 绑定后变为
  // const Camera* const&，即一个兼具顶层、底层 const 的指针的引用。
  const auto& 
    [camera, light_dir, shadow_res, slice_near, slice_far, scene_bbox_ws] = params;

  // ---------- 计算方向光的 world matrix（仅含旋转），并获得 light basis(lb) matrix
  Mat3f mat_light_rot_world = RotationMatrix::makeFromForward(light_dir);
  Mat3f mat_rot_lb = mat_light_rot_world.transpose();

  // ---------- 计算场景AABB在 lb 下的角点
  auto scene_bbox_corners_ws = scene_bbox_ws.corners();
  std::array<Vec3f, 8> scene_bbox_corners_lb;
  for(int i = 0; i < 8; i++)
    scene_bbox_corners_lb[i] = mat_rot_lb * scene_bbox_corners_ws[i];

  // ---------- 获取主相机视椎切片的外接球，将其直径作为 light ortho 正方形的边长
  Sphere cam_circumsphere_ws = camera->slicedFrustumCircumSphereWorld(slice_near, slice_far);

  f32 circumsphere_radius   = cam_circumsphere_ws.radius;
  f32 circumsphere_diameter = circumsphere_radius * 2;

  // ---------- Texel Snapping
  f32 world_units_per_texel = circumsphere_diameter / static_cast<f32>(shadow_res);

  Vec3f circumsphere_center_lb = mat_rot_lb * cam_circumsphere_ws.center;

  Vec2f light_ortho_center = circumsphere_center_lb.head<2>();

  // 将坐标对齐为 texel_size 的整数倍
  Vec2f light_ortho_center_snapped = (light_ortho_center / world_units_per_texel)
                                     .array()
                                     .round() * world_units_per_texel;

  // ---------- 求光视椎侧面与场景AABB的交集，取交集的 Zmin 和 Zmax 作为 near 和 far
  Vec2f light_ortho_min = light_ortho_center_snapped.array() - circumsphere_radius;
  Vec2f light_ortho_max = light_ortho_center_snapped.array() + circumsphere_radius;

  auto [near, far]
    = calcNearAndFar(light_ortho_min, light_ortho_max, scene_bbox_corners_lb);

  // ---------- 构建光源在 light basis 下的位置
  Vec3f light_pos_lb;
  light_pos_lb.head<2>() = light_ortho_center_snapped;
  light_pos_lb.z() = near - Camera::MinimumNearPlane;

  // 将平行光相机特化为 near 固定为 0.1.
  return {
    .light_pos_ws         = mat_light_rot_world * light_pos_lb,
    .ortho_near           = Camera::MinimumNearPlane,
    .ortho_far            = far - near,
    .ortho_width          = circumsphere_diameter,
    .ortho_height         = circumsphere_diameter,
    .shadow_texel_size_ws = world_units_per_texel
  };
}

} // namespace Medium

