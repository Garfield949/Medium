///
/// @file   DirectionalViewFit.h
/// @author Garfield949
/// @date   2026/4/9 12:22
/// @brief  
///
#pragma once

#include "Base/Types.h"

namespace Medium {
  
class Camera;
class DirectionalLight;

/// @struct DirectionalViewFitDesc
/// @brief  方向光阴影拟合参数描述。Desc 是 Description 的简写。
struct DirectionalViewFitDesc {
  const Camera* camera;
  const DirectionalLight* light;
  u32 shadow_res;
  f32 slice_far;
};

/// @struct DirectionalViewFit
/// @brief  方向光阴影拟合结果
/// @details 正交投影可直接将 near 设为 0，因为 “透视除法” 阶段顶点的 w 永远是 1，不会发生除零。
struct DirectionalViewFit {
  Vec3f circumsphere_center_snapped_ws;
  Vec3f light_pos;
  f32 dir_shadow_texel_size_ws;
  f32 ortho_near;
  f32 ortho_far;
  f32 ortho_width;
  f32 ortho_height;
};
  
} // namespace Medium

namespace Medium::ShadowMap {

/// @brief 根据参数计算返回方向光的视椎拟合结果
DirectionalViewFit calcDirectionalViewFit(const DirectionalViewFitDesc& desc);

// TODO: 将此 helper 作为 DirectionalLight 自身接口
/// @brief 设置方向光参数的 helper
void applyDirectionalViewFit(DirectionalLight* light, const DirectionalViewFit& fit);
  
} // namespace Medium::ShadowMap