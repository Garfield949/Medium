///
/// @file BlinnShader.h
/// @brief 布林冯着色器
///
#pragma once

#include "Pipeline/Shader/ShaderTypes.h"

namespace Medium {
  class Material;
  struct Vertex;
  struct CSMSamplingCtx;
  struct ShaderBindCtx;
  struct FSPayload;
}

namespace Medium {

struct BlinnShader {
  struct Uniform {
    Vec3f camera_pos; // 用于高光
    Mat4f view;       // 用于阴影级联选择
    Mat4f wvp;
    Mat4f world;
    const Material* material = nullptr;
    Vec3f light_dir_inv;
    const CSMSamplingCtx* csm_ctx = nullptr;
  };
  
  static constexpr u32 kOffsetUV      = 0;  // Vec2f
  static constexpr u32 kOffsetNormal  = 2;  // Vec3f
  static constexpr u32 kOffsetTangent = 5;  // Vec3f
  static constexpr u32 kOffsetDet     = 8;  // f32
  static constexpr u32 kOffsetPos     = 9;  // Vec3f
  
  static constexpr std::array<VaryingField, 5> kVaryingFields = {{
    {.offset = kOffsetUV,      .scalar_cnt = 2, .interp_mode = InterpMode::Perspective},
    {.offset = kOffsetNormal,  .scalar_cnt = 3, .interp_mode = InterpMode::Perspective},
    {.offset = kOffsetTangent, .scalar_cnt = 3, .interp_mode = InterpMode::Perspective},
    {.offset = kOffsetDet,     .scalar_cnt = 1, .interp_mode = InterpMode::Flat},
    {.offset = kOffsetPos,     .scalar_cnt = 3, .interp_mode = InterpMode::Perspective},
  }};

  static constexpr u32 varyingScalarCount() noexcept { return kOffsetPos + 3; }

  static constexpr VaryingLayout varyingLayout() noexcept { 
    return {
      .fields = kVaryingFields,
      .scalar_cnt = varyingScalarCount()
    };
  }
  
  void bind(const ShaderBindCtx& bind_ctx, const Mat4f& world, const Material* mat) noexcept;

  [[nodiscard]]
  VSOut vertex(const Vertex& vert) const noexcept;

  [[nodiscard]]
  bool fragment(FSPayload& fs_payload) const noexcept;

  Uniform uniforms;
};

} // namespace Medium