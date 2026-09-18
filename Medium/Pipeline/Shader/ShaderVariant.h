///
/// @file   ShaderVariant.h
/// @author Garfield949
/// @date   2026/3/28 14:20
/// @brief  注册 Shader 类型，以实现动态分发
///
#pragma once

#include <variant>

#include "BlinnShader.h"
#include "DepthShader.h"
#include "ImGuiShader.h"

namespace Medium {

using ShaderVariant = std::variant<BlinnShader, DepthShader, ImGuiShader>;

} // namespace Medium