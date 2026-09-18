///
/// @file   Light.h
/// @author Garfield949
/// @date   2026/3/15 23:02
/// @brief  
///
#pragma once

#include "Scene/Node.h"
#include "Base/Color.h"

namespace Medium {

class Light : public Node {
public:
  static constexpr NodeType Type = NodeType::Light;

  explicit Light(std::string name): Node{std::move(name), Type} {}

  LinearColor color = LinearColor::White();
  
  bool cast_shadows = true;
};

} // namespace Medium
