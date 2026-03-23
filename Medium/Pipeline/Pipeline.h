///
/// @file   Pipeline.h
/// @author Garfield949
/// @date   2026/3/1 20:40
/// @brief  
///
#pragma once

#include "Pipeline/RenderCommand.h"
#include "Pipeline/Shader/ShaderType.h"

namespace Medium {
  class  RenderTarget;
  class  Mesh;
  struct SubMesh;
  struct PassContext;
}

namespace Medium::Pipeline {

  /// @brief   视口变换
  /// @details 接收一个 NDC 坐标，将其 x,y 分量映射到 [0,width] [0,height]
  inline void NDCToScreen(Vec4f& coord, u32 sc_width, u32 sc_height) {
    coord.x() = 0.5f * static_cast<f32>(sc_width)  * (coord.x() + 1.0f);
    coord.y() = 0.5f * static_cast<f32>(sc_height) * (coord.y() + 1.0f);
  }
  
  void executeQueue(const PassContext& ctx, const std::vector<RenderCommand>& cmds);

  void draw(const Mesh&    mesh,
            const SubMesh& sub_mesh,
            VariantShader* shader,
            RenderTarget*  buffer);

  void rasterize(const std::array<Vec4f, 3>&   tri_screen,
                 const std::array<Varying, 3>& varyings,
                 VariantShader*                shader,
                 RenderTarget*                 buffer);
  
} // namespace Medium::Pipeline