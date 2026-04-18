///
/// @file   DirectionalViewFit.cpp
/// @author Garfield949
/// @date   2026/4/9 12:23
/// @brief  
///
#include "DirectionalViewFit.h"

#include "Base/Math/RotationMatrix.h"

#include "Scene/Light/DirectionalLight.h"

namespace Medium::ShadowMap {

DirectionalViewFit calcDirectionalViewFit(const DirectionalViewFitDesc& desc) {
  // 获取主相机视椎切片的外接球(world space)
  Sphere sphere = desc.camera->slicedFrustumCircumSphereWorld(desc.slice_far);

  // 外接球直径作为光源视椎的长宽尺寸
  f32 diameter = sphere.radius * 2;

  // Texel Snapping: 计算阴影贴图一个纹素占据多少个世界空间单位长度
  f32 world_units_per_texel = diameter / static_cast<f32>(desc.shadow_res);

  // 每帧重建光空间（旋转）
  Mat3f light_rot_world = RotationMatrix::makeFromForward(desc.light->dir);

  // 将球心变换到光空间，ls 代表 light space
  Mat3f light_view = light_rot_world.transpose();
  Vec3f center_snapped_ls = light_view * sphere.center;

  // 将坐标对齐为 texel_size 的整数倍
  center_snapped_ls.x() = std::round(center_snapped_ls.x() / world_units_per_texel) * world_units_per_texel;
  center_snapped_ls.y() = std::round(center_snapped_ls.y() / world_units_per_texel) * world_units_per_texel;

  // 球心转回世界空间，ws 代表 world space
  Vec3f center_snapped_ws = light_rot_world * center_snapped_ls;

  // 构建光源位置：从球心开始，往光源 dir 的反向进行 球半径 的位移.
  Vec3f light_pos = center_snapped_ws - desc.light->dir * (sphere.radius);
  
  return {
    center_snapped_ws,
    light_pos,
    world_units_per_texel,
    0.0f,
    diameter,
    diameter,
    diameter
  };
}

void applyDirectionalViewFit(DirectionalLight* light, const DirectionalViewFit& fit) {
  light->transform.setPosition(fit.light_pos);
  light->setOrthoParams(fit.ortho_near, fit.ortho_far, fit.ortho_width, fit.ortho_height);
}

} // namespace Medium::ShadowMap

