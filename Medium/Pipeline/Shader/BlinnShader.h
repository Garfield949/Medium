///
/// @file BlinnShader.h
/// @brief 布林冯着色器
///
#pragma once

#include "Pipeline/Shader/ShaderTypes.h"
#include "Resources/Image2D.h"

namespace Medium {

class Material;
class EnvironmentSH9;
struct Vertex;
struct CSMContext;
struct ShaderBindContext;
struct FSPayload;

struct BlinnShader {
  struct Uniform {
    Vec3f camera_pos;                         // 用于高光
    Mat4f mat_wvp;                            // 用于顶点着色器
    Mat4f mat_world;                          // 用于查询某些属性的世界空间位置
    const Material* material = nullptr;       // 当前着色点(SubMesh)绑定的材质

    Vec3f       light_dir_inv;                // 光照计算
    LinearColor light_color;                  // 光线颜色
    const EnvironmentSH9* env_sh9 = nullptr;  // 9系球谐函数，漫反射计算
    const Image2D<f32>* ao = nullptr;         // SSAO 缓冲

    const CSMContext* csm_ctx = nullptr;      // 级联阴影采样数据
  };

  Uniform uniforms;
  
  static constexpr i32 kOffsetNormal       = kOffsetUV      + 2;
  static constexpr i32 kOffsetTangent      = kOffsetNormal  + 3;
  static constexpr i32 kOffsetDet          = kOffsetTangent + 3;
  static constexpr i32 kOffsetPos          = kOffsetDet     + 1;
  static constexpr i32 kVaryingScalarCount = kOffsetPos     + 3;
  
  void bind(const ShaderBindContext& bind_ctx, const Mat4f& world, const Material* mat) noexcept;

  [[nodiscard]]
  VSOut vertex(const Vertex& vert) const noexcept;

  [[nodiscard]]
  bool fragment(const FSIn& fs_in, LinearColor& frag_color) const noexcept;

  void enableMipmap(bool enable) noexcept { mipmap_enabled_ = enable; }
  bool mipmapEnabled() const noexcept { return mipmap_enabled_; }

private:
  bool mipmap_enabled_ = true;
};

} // namespace Medium