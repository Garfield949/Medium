///
/// @file   CSM.h
/// @author Garfield949
/// @date   2026/4/20 21:29
/// @brief  级联阴影映射 Cascaded Shadow Mapping
/// @details 注：Texture Atlas 是指纹理图集，将多张不同纹理合并到一张大纹理贴图中。
///
#pragma once

#include "Base/Types.h"
#include "Base/Geometry/Rect.h"

namespace Medium {

class FrameBuffer;

inline constexpr i32 kMaxCSMCascades = 4;

/// @struct CSMCascade
/// @brief 记录单个级联的上下文。
struct CSMCascade {
  // 记录级联的逻辑切片范围。
  f32 slice_near = 0.0f;
  f32 slice_far  = 0.0f;
  // 记录级联用于光视椎拟合的切片范围，主要起效的是 fit_near. 当该级联非第 0 级时，将其 fit_near 置为
  // slice_near - CSM blend band width, 让该级联的采样范围往 near 往前扩张一些，防止在级联混合阶段
  // 下一级联的 shadow map 无法完全覆盖 blend band 导致采样不稳定。
  f32 fit_near = 0.0f;
  f32 fit_far  = 0.0f;

  // 各级联在光空间中的 ViewFit 不同，故各自绑定对应的变换矩阵。
  Mat4f light_vp;
  RectF viewport;

  f32 texel_size_ws = 0.0f;

  f32 width() const noexcept {
    return slice_far - slice_near;
  }
};

/// @struct CSMSamplingCtx
/// @brief 存储级联数据的结构体，用于 fragment 查询阶段。
struct CSMSamplingCtx {
  std::array<CSMCascade, kMaxCSMCascades> cascades;
  const FrameBuffer* shadow_map_atlas = nullptr;
  i32 cascade_cnt = 0;

  struct CascadeQuery {
    const CSMCascade* cascade = nullptr;
    int index = 0;
  };

  // 返回级联本身和所在层级。
  std::optional<CascadeQuery> cascadeQuery(f32 z_view) const noexcept {
    for(i32 i = 0; i < cascade_cnt; ++i) {
      const CSMCascade& c = cascades[i];
      if(z_view >= c.slice_near && z_view <= c.slice_far) return CascadeQuery{&c, i};
    }
    return std::nullopt;
  }

  f32 blendBandWidth(int cascade_index) const noexcept {
    f32 blend_factor = 0.2f;
    return blend_factor * cascades[cascade_index].width();
  }
};

/// @brief 计算各级联的切片逻辑范围。
/// @details 假设 CSM 级联数为 2，则：
///          CSM 0: camera_near -> slice_far0.
///          CSM 1: slice_far0  -> max_shadow_dist.
///          需存储 3 个切分点 camera_near, slice_far0, max_shadow_dist. 故返回的数组长度为 kMaxCSMCascades + 1.
std::array<f32, kMaxCSMCascades + 1> calcCSMSlices(f32 camera_near,
                                                   f32 max_shadow_dist,
                                                   u32 cascade_cnt);

/// @brief 计算 CSM 级联的视口。
RectF calcCSMAtlasViewport(i32 cascade_index, u32 tile_res);

} // namespace Medium
