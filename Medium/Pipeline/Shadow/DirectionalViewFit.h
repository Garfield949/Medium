///
/// @file   DirectionalViewFit.h
/// @author Garfield949
/// @date   2026/4/9 12:22
/// @brief  
///
#pragma once

#include "Base/Geometry/AABB3D.h"

namespace Medium {
  
class Camera;

/// @struct DirectionalViewFitParams
struct DirectionalViewFitParams {
  const Camera* main_camera = nullptr;
  Vec3f light_dir;
  i32 shadow_res;
  f32 slice_near;
  f32 slice_far;
  AABB3D scene_bbox_ws;
};

/// @struct DirectionalViewFit
/// @brief  平行光阴影拟合结果
struct DirectionalViewFit {
  Vec3f light_pos_ws;
  f32 ortho_far;
  f32 ortho_width;
  f32 ortho_height;
  f32 shadow_texel_size_ws;
};

/// @brief 根据参数计算返回平行光的视椎拟合结果
DirectionalViewFit calcDirectionalViewFit(const DirectionalViewFitParams& params);
  
} // namespace Medium