///
/// @file   ShadowMap.h
/// @author Garfield949
/// @date   2026/4/9 12:22
/// @brief  
///
#pragma once

#include "Base/Types.h"

namespace Medium {
  
  class Camera;
  class DirectionalLight;

  /// @struct DirectionalShadowFitDesc
  /// @brief  方向光阴影拟合参数描述
  struct DirectionalShadowFitDesc {
    const Camera* camera;
    const DirectionalLight* light;
    u32 shadow_map_res;
    f32 slice_far;
  };
  
  /// @struct DirectionalShadowFitRes
  /// @brief  方向光阴影拟合结果
  struct DirectionalShadowFitRes {
    Vec3f circumsphere_center_snapped_ws;
    Vec3f light_pos;
    f32 ortho_near;
    f32 ortho_far;
    f32 ortho_width;
    f32 ortho_height;
  };
  
} // namespace Medium

namespace Medium::ShadowMap {
  
  DirectionalShadowFitRes directionalShadowFit(const DirectionalShadowFitDesc& desc);

  void applyDirectionalShadowFit(DirectionalLight* light, const DirectionalShadowFitRes& fit);
  
} // namespace Medium::ShadowMap