///
/// @file   Material.h
/// @author Garfield949
/// @date   2026/2/14 11:42
/// @brief
///
#pragma once

#include "Base/Color.h"
#include "Resources/Forwards.h"
#include "Resources/TextureSlot.h"

namespace Medium {

struct MaterialParams {
  // 材质颜色与强度参数。
  // 前缀 "K" 代表「系数」，三个分量对应 R, G, B 三个通道的强度；
  // 1. Diffuse,   漫反射系数；
  // 2. Specular,  镜面反射系数，决定高光强度；
  LinearColor Kd = {1.0f};
  LinearColor Ks = {1.0f};
  // 表面属性参数，控制光线与表面交互的细节。
  // 1. Shininess, 亮泽度，决定了高光的聚散（清晰度）程度；
  // 2. Dissolve, 不透明度，1.0 表示完全不透明。
  f32 Ns = 0.0f;
  f32 d  = 1.0f;
  // Illumination Model, 控制光照的计算公式。
  // 0 = 无光照，直接输出 Kd；
  // 1 = 漫反射；
  // 2 = 漫反射 + 高光；
  i32 illum = 0;
};

class Material {
  friend class AssetCache;
public:
  enum class SurfaceMode {
    Opaque,     // MTL 不含 map_d 且 d = 1;
    Masked,     // MTL 含 map_d;
    Transparent // MTL 里 d < 1，与 Masked 互斥。
  };

  // map_d 一般是一张非黑即白的二值图，所以 alpha test 的阈值设为该值即可；cut off 是切断、截止的意思。
  static constexpr f32 AlphaCutoff = 0.5f;
  
  Material(std::string mat_name = "Default") : name{std::move(mat_name)} {}

  [[nodiscard]]
  static std::shared_ptr<Material> Default() {
    return std::make_shared<Material>();
  }

  void setAttributes() noexcept;

  std::string name;

  MaterialParams params;

  SurfaceMode surface_mode = SurfaceMode::Opaque;

  bool enable_spec = false;
  bool enable_norm = false;

  TextureSlot diff_slot = LinearColor::Green();
  TextureSlot spec_slot = LinearColor::Black();

  TexturePtr diss_slot = nullptr;
  TexturePtr norm_slot = nullptr;
};

} // Medium
