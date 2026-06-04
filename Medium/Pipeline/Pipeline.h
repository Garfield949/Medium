///
/// @file   Pipeline.h
/// @author Garfield949
/// @date   2026/3/1 20:40
/// @brief  模拟硬件行为，不包含状态
///
#pragma once

#include "Pipeline/RenderCommand.h"
#include "Pipeline/Shader/VariantShader.h"

namespace Medium {
class  RenderTarget;
class  Mesh;
struct SubMesh;
struct ShaderBindCtx;
struct RenderPassCtx;
struct RectF;
struct RenderState;
}

namespace Medium::Pipeline {

void executeCommands(const std::vector<RenderCommand>& cmds,
                     const ShaderBindCtx& shader_ctx,
                     const RenderPassCtx& pass_ctx,
                     VariantShader&       shader);

void draw(const Mesh&        mesh,
          const SubMesh&     sub_mesh,
          const RenderState& render_state,
          const RectF&       viewport,
          RenderTarget&      buffer,
          VariantShader&     shader);

void rasterize(const std::array<Vec4f, 3>&   tri_screen,
               const std::array<Varying, 3>& varyings,
               const RenderState& render_state,
               const RectF&       viewport,
               RenderTarget&      buffer,
               VariantShader&     shader);
  
} // namespace Medium::Pipeline