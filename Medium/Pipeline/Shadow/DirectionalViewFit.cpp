///
/// @file   DirectionalViewFit.cpp
/// @author Garfield949
/// @date   2026/4/9 12:23
/// @brief  
///
#include "DirectionalViewFit.h"

#include "Base/Math/Math.h"
#include "Base/Math/RotationMatrix.h"
#include "Scene/Light/DirectionalLight.h"

namespace {

using namespace Medium;

// 定义裁剪面枚举
enum FrustumSide {
  Left   = 1 << 0,
  Right  = 1 << 1,
  Top    = 1 << 2,
  Bottom = 1 << 3,
  None   = 0
};

int calcOutcode(const Vec2f& light_ortho_min,
                        const Vec2f& light_ortho_max,
                        const Vec3f& v) noexcept {
  int code = None;
  if(v.x() < light_ortho_min.x()) code |= Left;
  if(v.x() > light_ortho_max.x()) code |= Right;
  if(v.y() > light_ortho_max.y()) code |= Top;
  if(v.y() < light_ortho_min.y()) code |= Bottom;
  return code;
}

// 三角形初始有 3 个顶点，每次与裁剪面相交最多新产生 1 个顶点；遍历 4 个面便会最多得到 7 个顶点
constexpr size_t kMaxClipVertsNum = 7;
struct ClipPolygon {
  std::array<Vec3f, kMaxClipVertsNum> verts;
  size_t num_verts = 0;
};

[[nodiscard]] f32 calcSignedDistance(const Vec2f& light_ortho_min,
                                     const Vec2f& light_ortho_max,
                                     const Vec3f& v, FrustumSide plane) noexcept {
  switch(plane) {
    case Left:   return v.x() - light_ortho_min.x();
    case Right:  return light_ortho_max.x() - v.x();
    case Top:    return light_ortho_max.y() - v.y();
    case Bottom: return v.y() - light_ortho_min.y();
    default: return 0.0f;
  }
}

[[nodiscard]] ClipPolygon clipByPlane(const Vec2f& light_ortho_min,
                                      const Vec2f& light_ortho_max,
                                      const ClipPolygon& poly_in,
                                      const FrustumSide  plane) noexcept {
  ClipPolygon poly_out;

  // 遍历输入多边形的所有边，由 head -> tail 定义
  for(size_t i = 0; i < poly_in.num_verts; ++i) {
    // 当 i = num_verts - 1, (i + 1) % num_verts = 0，尾首完成闭环
    const Vec3f* v_head = &poly_in.verts[i];
    const Vec3f* v_tail = &poly_in.verts[(i + 1) % poly_in.num_verts];

    f32 d_head = calcSignedDistance(light_ortho_min, light_ortho_max, *v_head, plane);
    f32 d_tail = calcSignedDistance(light_ortho_min, light_ortho_max, *v_tail, plane);

    bool inside_head = d_head >= 0.0f;
    bool inside_tail = d_tail >= 0.0f;

    // 当前顶点若在裁剪面内则直接加入多边形
    if(inside_head) poly_out.verts[poly_out.num_verts++] = *v_head;

    // 若当前、下一个顶点不在同侧，说明此边与裁剪面有交点
    if(inside_head != inside_tail)
      poly_out.verts[poly_out.num_verts++] = Math::lerp(*v_head, *v_tail, d_head / (d_head - d_tail));
  }

  return poly_out;
}

constexpr std::array kClipPlanes {
  Left, Right, Top, Bottom
};

[[nodiscard]] bool clip(const Vec2f& light_ortho_min,
                        const Vec2f& light_ortho_max,
                        ClipPolygon& tri) noexcept {
  int code0 = calcOutcode(light_ortho_min, light_ortho_max, tri.verts[0]);
  int code1 = calcOutcode(light_ortho_min, light_ortho_max, tri.verts[1]);
  int code2 = calcOutcode(light_ortho_min, light_ortho_max, tri.verts[2]);

  // 三个顶点均在某裁剪面外侧，不可见直接丢弃
  if((code0 & code1 & code2) != None) return false;

  int union_code = code0 | code1 | code2;
  // 三个顶点都不在任意一个裁剪面外部，则面片在视椎内，无需裁剪
  if(union_code == None) return true;

  for(const auto& plane: kClipPlanes) {
    if((union_code & plane) == None) continue;
    tri = clipByPlane(light_ortho_min, light_ortho_max, tri, plane);
    if(tri.num_verts < 3) return false;
  }
  return true;
}

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

DirectionalViewFit stableFit(const DirectionalViewFitDesc& fit_desc) {
  // ---------- 计算方向光的 world matrix（仅含旋转），并获得 light basis(lb) matrix
  Mat3f mat_light_rot_world = RotationMatrix::makeFromForward(fit_desc.light_dir);
  Mat3f mat_rot_lb = mat_light_rot_world.transpose();

  // ---------- 计算场景AABB在 lb 下的角点
  auto scene_bbox_corners_ws = fit_desc.bbox_scene_ws.corners();
  std::array<Vec3f, 8> scene_bbox_corners_lb;
  for(int i = 0; i < 8; i++)
    scene_bbox_corners_lb[i] = mat_rot_lb * scene_bbox_corners_ws[i];

  // ---------- 获取主相机视椎切片的外接球，将其直径作为 light ortho 正方形的边长
  Sphere cam_circumsphere_ws = fit_desc.camera->slicedFrustumCircumSphereWorld(fit_desc.slice_far);

  f32 radius   = cam_circumsphere_ws.radius;
  f32 diameter = radius * 2;

  // ---------- Texel Snapping
  f32 world_units_per_texel = diameter / static_cast<f32>(fit_desc.shadow_res);

  Vec3f circumsphere_center_lb = mat_rot_lb * cam_circumsphere_ws.center;

  Vec2f light_ortho_center = circumsphere_center_lb.head<2>();

  // 将坐标对齐为 texel_size 的整数倍
  Vec2f light_ortho_center_snapped = (light_ortho_center / world_units_per_texel)
                                     .array()
                                     .round() * world_units_per_texel;

  // ---------- 求光视椎侧面与场景AABB的交集，取交集的 Zmin 和 Zmax 作为 near 和 far
  Vec2f light_ortho_min = light_ortho_center_snapped.array() - radius;
  Vec2f light_ortho_max = light_ortho_center_snapped.array() + radius;

  auto [near, far]
    = calcNearAndFar(light_ortho_min, light_ortho_max, scene_bbox_corners_lb);

  Vec3f dir_light_pos_lb;
  dir_light_pos_lb.head<2>() = light_ortho_center_snapped;
  dir_light_pos_lb.z() = near;

  return {
    .light_pos_ws         = mat_light_rot_world * dir_light_pos_lb,
    .ortho_near           = 0.0f,
    .ortho_far            = far - near,
    .ortho_width          = diameter,
    .ortho_height         = diameter,
    .shadow_texel_size_ws = world_units_per_texel
  };
} // stableFit

} // 匿名命名空间，控制仅单文件可见


namespace Medium {

DirectionalViewFit calcDirectionalViewFit(const DirectionalViewFitDesc& fit_desc) {
  return stableFit(fit_desc);
}

} // namespace Medium

