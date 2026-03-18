///
/// @file   ShaderType.h
/// @author Garfield949
/// @date   2026/3/2 13:17
/// @brief
///
#pragma once

#include <variant>

#include "BlinnPhong.h"
#include "ShadowShader.h"

namespace Medium {

  using VariantShader = std::variant<BlinnPhongShader, ShadowShader>;

} // namespace Medium
