///
/// @file   DrawCore.h
/// @author Garfield949
/// @date   2026/3/1 20:40
/// @brief  模拟硬件行为，不包含状态
///
#pragma once

#include <span>

#include "Pipeline/Shader/ShaderVariant.h"

namespace Medium {
class  FrameBuffer;
struct ShaderBindContext;
struct RenderPassContext;
struct RectF;
struct RenderState;
struct VaryingLayout;
struct DrawCommand;
}

namespace Medium::Pipeline {

/// @struct IndexedDrawView
/// @brief 描述一段待绘制的几何视图。
/// @note Geo 代表 Geometry.
struct IndexedDrawView {
  std::span<const Vertex> vertices;
  std::span<const size_t> indices;
  size_t index_offset;
  size_t index_count;
  size_t vertex_offset;
  // ImGui 的新增支持项，给定一个 vertices 的基偏移量，例子如下：
  // vertices: [0 ... 999], indices: {0, 1, 2}, vertex_offset = 500.
  // 那么 indices 指代的三角形为：
  // Tri = vertices[500 + 0 = 500], vertices[500 + 1 = 501], vertices[500 + 2 = 502]. 
};

/// @brief 执行各 Pass 所组织 DrawQueue 吐出的命令队列。
void executeDrawCommands(std::span<const DrawCommand> cmds,
                         const ShaderBindContext&     shader_ctx,
                         const RenderPassContext&     pass_ctx,
                         ShaderVariant&               shader);

/// @brief Drawcall 实现，移除 SubMesh 抽象。
void draw(const IndexedDrawView& draw_view,
          const RenderState&     render_state,
          const RectF&           viewport,
          FrameBuffer&           buffer,
          ShaderVariant&         shader,
          RenderStatus*          render_status);

/// @brief 光栅化填充模式，Edge Equation.
void rasterizeTriangle(const std::array<Vec4f, 3>&   triangle,
                       const std::array<Varying, 3>& varyings,
                       const VaryingLayout&          varying_layout,
                       const RenderState&            render_state,
                       const RectF&                  viewport,
                       FrameBuffer&                  buffer,
                       ShaderVariant&                shader,
                       RenderStatus*                 render_status);

/// @brief 绘制线段，输入两个 ndc 线段端点。
bool rasterizeLine(Vec4f&             v0_ndc,
                   Vec4f&             v1_ndc,
                   const RectF&       viewport,
                   FrameBuffer&       buffer,
                   const LinearColor& color,
                   RenderStatus*      render_status);
  
} // namespace Medium::Pipeline