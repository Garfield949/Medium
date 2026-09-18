///
/// @file   ShaderTypes.h
/// @author Garfield949
/// @date   2026/6/15 13:25
/// @brief
///
#pragma once

#include <array>
#include <span>

#include "Base/Color.h"
#include "Base/Types.h"

namespace Medium {

inline constexpr i32 kMaxVaryingScalars = 16;

enum class InterpMode {
  Perspective,
  NoPerspective,
  Flat
};

/// @struct VaryingField
/// @brief  描述 Varying 中单个域的偏移和元信息，比如 uv.
struct VaryingField {
  i32 offset = 0;
  i32 scalar_cnt = 0;
  bool calc_grad = false;
  InterpMode interp_mode = InterpMode::Perspective;
};

/// @struct VaryingLayout
/// @brief  携带 Varying 中所有域的布局。
struct VaryingLayout {
  std::span<const VaryingField> fields;
  i32 scalar_cnt = 0;
};

/// @struct Varying
/// @brief  存储 Varying 中所有域的离散标量数据。
struct Varying {
  void store(const f32* vec_data, i32 offset, i32 scalar_cnt) {
    for(i32 i = 0; i < scalar_cnt; ++i)
      scalars[offset + i] = vec_data[i];
  }
  std::array<f32, kMaxVaryingScalars> scalars;
  // 不显式指定的话，默认使用 1.0f 会退化为线性插值。
  // 将 w_inv 保存进 Varying 的原因是，rasterizeTriangle 阶段会将 Vec4f 的顶点量化为 Vec3i 的顶点数，会丢掉 w() 分量。
  f32 w_inv = 1.0f;
};

/// @struct VSOut
/// @brief  顶点着色器输出数据。
struct VSOut {
  Vec4f pos_clip;
  Varying varying;
};

/// @struct FSIn
/// @brief  片段着色器输入数据。
struct FSIn {
  [[nodiscard]] Vec2f getVec2f(i32 offset) const noexcept {
    return { 
      scalars_interp[offset + 0], 
      scalars_interp[offset + 1]
    };
  }
  [[nodiscard]] Vec3f getVec3f(i32 offset) const noexcept {
    return { 
      scalars_interp[offset + 0], 
      scalars_interp[offset + 1], 
      scalars_interp[offset + 2] 
    };
  }
  [[nodiscard]] LinearColor getLinearColor(i32 offset) const noexcept {
    return {
      scalars_interp[offset + 0], 
      scalars_interp[offset + 1], 
      scalars_interp[offset + 2],
      scalars_interp[offset + 3] 
    };
  }
  [[nodiscard]] f32 getScalar(i32 offset) const noexcept {
    return scalars_interp[offset];
  }
  std::array<f32, kMaxVaryingScalars> scalars_interp;
};

} // namespace Medium