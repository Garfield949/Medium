///
/// @file BlinnShader.h
/// @brief 布林冯着色器
///
#pragma once

#include "Pipeline/Shader/ShaderTypes.h"

namespace Medium {
  class Material;
  struct Vertex;
  struct CSMContext;
  struct ShaderBindContext;
  struct FSPayload;
}

namespace Medium {

struct BlinnShader {
  struct Uniform {
    Vec3f camera_pos;                     // 用于高光
    Mat4f mat_wvp;                        // 用于顶点着色器
    Mat4f mat_world;                      // 用于查询某些属性的世界空间位置
    const Material* material = nullptr;   // 当前着色点(SubMesh)绑定的材质

    Vec3f       light_dir_inv;            // 光照计算
    LinearColor light_color;              // 光线颜色

    const CSMContext* csm_ctx = nullptr;  // 级联阴影采样数据
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
  
  void bind(const ShaderBindContext& bind_ctx, const Mat4f& world, const Material* mat) noexcept;

  [[nodiscard]]
  VSOut vertex(const Vertex& vert) const noexcept;

  [[nodiscard]]
  bool fragment(FSPayload& fs_payload) const noexcept;

  Uniform uniforms;
};

} // namespace Medium