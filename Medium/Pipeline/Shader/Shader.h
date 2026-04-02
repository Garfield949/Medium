///
/// @file   Shader.h
/// @author Garfield949
/// @date   2026/3/28 14:20
/// @brief  
///
#pragma once

#include <variant>

#include "BlinnShader.h"
#include "DepthShader.h"

namespace Medium {

  using VariantShader = std::variant<BlinnShader, DepthShader>;

} // namespace Medium