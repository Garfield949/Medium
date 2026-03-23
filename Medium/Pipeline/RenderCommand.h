///
/// @file   RenderCommand.h
/// @author Garfield949
/// @date   2026/3/18 23:33
/// @brief  
///
#pragma once

#include "Base/Types.h"

namespace Medium {

  class Material;
  class Mesh;
  struct SubMesh;

  struct RenderCommand {
    const Mesh*     mesh         = nullptr;
    const SubMesh*  submesh      = nullptr;
    const Material* material     = nullptr;
    const Mat4f*    matrix_world = nullptr;
  };

} // namespace Medium
