///
/// @file   Material.h
/// @author Garfield949
/// @date   2026/2/14 11:42
/// @brief
///
#pragma once

#include <utility>

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
  Material(std::string mat_name = "Default") : name(std::move(mat_name)) {}

  static std::shared_ptr<Material> Default() {
    return std::make_shared<Material>();
  }

  void setEnables() noexcept;

  std::string name;

  MaterialParams params;

  bool enable_spec = false;
  bool enable_diss = false;
  bool enable_norm = false;
  bool enable_bump = false;

  TextureSlot diff_slot = LinearColor::Green();
  TextureSlot spec_slot = LinearColor::Black();
  TextureSlot diss_slot = LinearColor::White();

  TexturePtr norm_slot = nullptr;
  TexturePtr bump_slot = nullptr;
};

} // Medium
