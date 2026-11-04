///
/// @file   ShaderTypes.h
/// @author Garfield949
/// @date   2026/6/15 13:25
/// @brief
///
#pragma once

#include <array>
#include <cstddef>

#include "Base/Color.h"
#include "Base/Types.h"

namespace Medium {

inline constexpr i32 kOffsetUV = 0;
inline constexpr i32 kMaxVaryingScalars = 16;

/// @struct Varying
/// @brief  存储 Varying 中所有域的离散标量数据。
struct Varying {
  void store(const f32* vec_data, i32 offset, i32 scalar_cnt) {
    for(i32 i = 0; i < scalar_cnt; ++i)
      scalars[offset + i] = vec_data[i];
  }
  std::array<f32, kMaxVaryingScalars> scalars;
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

  // Varyings 的平铺插值数据。
  std::array<f32, kMaxVaryingScalars> scalars_interp;

  // uv 屏幕导数。
  Vec2f duv_dx;
  Vec2f duv_dy;

  size_t pixel_index;
};

} // namespace Medium