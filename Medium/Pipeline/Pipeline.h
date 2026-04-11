///
/// @file   Pipeline.h
/// @author Garfield949
/// @date   2026/3/1 20:40
/// @brief  
///
#pragma once

#include "Pipeline/RenderCommand.h"
#include "Pipeline/Shader/Shader.h"

namespace Medium {
  class  RenderTarget;
  class  Mesh;
  struct SubMesh;
  struct PassCtx;
}

namespace Medium::Pipeline {

  void executeCommands(const std::vector<RenderCommand>& cmds, 
                       const PassCtx& ctx);

  void draw(const Mesh&    mesh,
            const SubMesh& sub_mesh,
            VariantShader* shader,
            RenderTarget*  buffer);

  void rasterize(const std::array<Vec4f, 3>&   tri_screen,
                 const std::array<Varying, 3>& varyings,
                 VariantShader* shader,
                 RenderTarget*  buffer);
  
} // namespace Medium::Pipeline