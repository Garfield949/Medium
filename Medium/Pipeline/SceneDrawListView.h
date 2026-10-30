///
/// @file   SceneDrawListView.h
/// @author Garfield949
/// @date   2026/10/13 21:03
/// @brief  
///
#pragma once

#include <span>

#include "DrawCommand.h"

namespace Medium {

struct SceneDrawListView {
  std::span<const DrawCommand> opaque;
  std::span<const DrawCommand> transparent;
};

} // namespace Medium