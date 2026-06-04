///
/// @file   Material.cpp
/// @author Garfield949
/// @date   2026/3/9 09:25
/// @brief  
///
#include "Material.h"

namespace Medium {

void Material::setAttributes() noexcept {
  // std::holds_alternative 检查一个 std::variant 当前存放的是不是指定的类型。
  bool spec_map_exists = std::holds_alternative<TexturePtr>(spec_slot);
  bool diss_map_exists = diss_slot != nullptr;
  bool norm_map_exists = norm_slot != nullptr;

  enable_spec = (spec_map_exists || !params.Ks.isBlack()) && (params.illum >= 2);
  enable_norm = norm_map_exists;

  if(diss_map_exists)
    surface_mode = SurfaceMode::Masked;
  else if(!Math::isNearlyOne(params.d))
    surface_mode = SurfaceMode::Transparent;
  else
    surface_mode = SurfaceMode::Opaque;
}

} // namespace Medium
