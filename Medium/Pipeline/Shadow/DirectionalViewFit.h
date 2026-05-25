///
/// @file   DirectionalViewFit.h
/// @author Garfield949
/// @date   2026/4/9 12:22
/// @brief  
///
#pragma once

#include "Geometry/AABB.h"
#include "Scene/Camera.h"

namespace Medium {
  
class Camera;
class DirectionalLight;

/// @struct DirectionalViewFitDesc
/// @brief  平行光阴影拟合参数描述。Desc 是 Description 的简写。
struct DirectionalViewFitDesc {
  const Camera* camera = nullptr;
  Vec3f light_dir;
  u32 shadow_res = 0u;
  f32 slice_far = 0.0f;
  AABB3D bbox_scene_ws;
};

/// @struct DirectionalViewFit
/// @brief  平行光阴影拟合结果
/// @details 正交投影可直接将 near 设为 0，因为 “透视除法” 阶段顶点的 w 永远是 1，不会发生除零。
struct DirectionalViewFit {
  Vec3f light_pos_ws;
  f32 ortho_near;
  f32 ortho_far;
  f32 ortho_width;
  f32 ortho_height;
  f32 shadow_texel_size_ws;
};

/// @brief 根据参数计算返回平行光的视椎拟合结果
DirectionalViewFit calcDirectionalViewFit(const DirectionalViewFitDesc& fit_desc);
  
} // namespace Medium