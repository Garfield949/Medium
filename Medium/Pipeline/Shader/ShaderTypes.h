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

struct VaryingField {
  i32 offset = 0;
  i32 scalar_cnt = 0;
  InterpMode interp_mode = InterpMode::Perspective;
};

struct VaryingLayout {
  std::span<const VaryingField> fields;
  i32 scalar_cnt = 0;
};

struct Varying {
  void store(const f32* vec_data, i32 offset, i32 scalar_cnt) {
    for(i32 i = 0; i < scalar_cnt; ++i)
      scalars[offset + i] = vec_data[i];
  }
  std::array<f32, kMaxVaryingScalars> scalars;
  f32 w_inv = 1.0f; // 不显式指定的话，会退化为线性插值。
};

struct VSOut {
  Vec4f pos_clip;
  Varying varying;
};

struct FSPayload {
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
  LinearColor frag_color;
};

} // namespace Medium