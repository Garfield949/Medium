///
/// @file   ImGuiShader.cpp
/// @author Garfield949
/// @date   2026/6/19 15:44
/// @brief  
///

#include "ImGuiShader.h"

namespace Medium {

void ImGuiShader::bind(Vec2f display_pos,Vec2f display_size, const Texture2D* texture) noexcept {
  uniform.display_pos  = display_pos;
  uniform.display_size = display_size;
  uniform.texture      = texture;
}

} // namespace Medium