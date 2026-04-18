///
/// @file   Material.cpp
/// @author Garfield949
/// @date   2026/3/9 09:25
/// @brief  
///
#include "Material.h"

#include <fmt/color.h>

namespace Medium {

void Material::setEnables() noexcept {
  bool spec_map_exists = std::holds_alternative<TexturePtr>(spec_slot);
  bool diss_map_exists = std::holds_alternative<TexturePtr>(diss_slot);
  bool norm_map_exists = norm_slot != nullptr;
  bool bump_map_exists = bump_slot != nullptr;

  enable_spec = (spec_map_exists || !params.Ks.isBlack()) && (params.illum >= 2);
  enable_diss = diss_map_exists || !Math::isNearlyOne(params.d);
  enable_norm = norm_map_exists;
  enable_bump = bump_map_exists;
  
  // fmt::print(fmt::emphasis::bold | fg(fmt::color::green),
  //        "\nMaterial: {}\n", name);
  // fmt::print("Specular enabled: {}\n", enable_spec);
  // fmt::print("Dissolve enabled: {}\n", enable_diss);
  // fmt::print("Normal enabled: {}\n", enable_norm);
  // fmt::print("Bump enabled: {}\n", enable_bump);
}

} // namespace Medium
