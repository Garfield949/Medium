///
/// @file   Forwards.h
/// @author Garfield949
/// @date   2026/2/19 23:56
/// @brief  
///
#pragma once

#include <memory>

namespace Medium {

class Mesh;
class Texture2D;
class Material;

using TexturePtr  = std::shared_ptr<Texture2D>;
using MaterialPtr = std::shared_ptr<Material>;
using MeshPtr     = std::shared_ptr<Mesh>;

} // namespace Medium
