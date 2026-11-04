///
/// @file   DebugRenderer.cpp
/// @author Garfield949
/// @date   2026/7/3 23:30
/// @brief  
///
#include "DebugRenderer.h"

#include "Base/Geometry/Rect.h"
#include "Pipeline/RenderContext.h"
#include "Resources/Mesh.h"

#include "Clip.h"
#include "DrawCore.h"
#include "FrameBuffer.h"
#include "DrawCommand.h"
#include "RenderStatus.h"
#include "SceneDrawListView.h"

namespace Medium {

void DebugRenderer::drawAABBs(Mat4f mat_vp,
                              const SceneDrawListView& draw_list,
                              FrameBuffer&  frame_buffer,
                              RenderStatus& render_status) 
{
  // 注意索引要对照 AABB3D::corners() 的实现。
  constexpr std::array<u8, 24> line_indices = {
    0, 1,  1, 2,  2, 3,  3, 0,
    4, 5,  5, 6,  6, 7,  7, 4,
    0, 4,  1, 5,  3, 7,  2, 6
  };

  RectF viewport = {0, 0, frame_buffer.widthF(), frame_buffer.heightF()};

  // 实际线段绘制只用得到 depth_cmp.
  RenderState render_state {
    .enable_depth_test  = true,
    .enable_depth_write = true,
    .enable_color_write = true,
    .draw_mode          = DrawMode::Shaded,
    .cull_mode          = CullMode::Back,
    .depth_cmp          = CompareOP::Greater,
    .blend_mode         = BlendMode::Replace,
    .color_write_target = ColorWriteTarget::HDR
  };

  auto foo = [&](const DrawCommand& cmd) {
    // 获取各 submesh 的世界控件包围盒角点。
    std::array<Vec3f, 8> bbox_corners_world
      = cmd.submesh->aabb_local.toWorldAABB(cmd.mat_world).corners();

    // 将角点变换到裁剪空间。
    std::array<Vec4f, 8> bbox_corners_clip;
    for(size_t i = 0; i < 8; ++i)
      bbox_corners_clip[i] = mat_vp * bbox_corners_world[i].homogeneous();

    // 裁剪 12 条边并送入光栅化。
    for(size_t i = 0; i < 12; ++i) {
      Vec4f v0 = bbox_corners_clip[line_indices[2 * i + 0]];
      Vec4f v1 = bbox_corners_clip[line_indices[2 * i + 1]];

      if(!clipLine(v0, v1)) continue;

      // Clip to NDC.
      v0 /= v0.w();
      v1 /= v1.w();

      Pipeline::rasterizeLine(v0, v1, render_state, viewport, LinearColor::Green(), frame_buffer, &render_status);
    }
  };

  // 复用 SceneRenderer 已经做过视椎剔除的 SubMesh 命令队列。
  for(const auto& cmd: draw_list.opaque) foo(cmd);
  for(const auto& cmd: draw_list.transparent) foo(cmd);
}

} // namespace Medium