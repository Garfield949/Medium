///
/// @file   MeshNode.cpp
/// @author Garfield949
/// @date   2026/2/13 22:58
/// @brief
///
#include "MeshNode.h"

#include <iostream>

#include "Resources/Mesh.h"
#include "Pipeline/RenderQueue.h"
#include "Pipeline/RenderContext.h"

namespace Medium {
  
  void MeshNode::commitCommand(RenderQueue& queue, const ViewCtx& view_ctx) const {
    const Mat4f& world = transform.worldMatrix();
    const Frustum& frustum = view_ctx.frustum;
    
    if(frustum.intersects(mesh_->aabb.toWorldAABB(world))) {
      for(const auto& sub_mesh : mesh_->sub_meshes) {
        AABB3D bbox_world = sub_mesh.bbox.toWorldAABB(world);
        if(!frustum.intersects(bbox_world)) continue;
        queue.collectCommands({
          mesh_.get(),
          &sub_mesh,
          mesh_->materials[sub_mesh.material_index].get(),
          &world,
          // TODO: 重新设计 sort key
          (view_ctx.view * bbox_world.center().homogeneous()).z()
        });
      }
    }
    
    Node::commitCommand(queue, view_ctx);
  }

} // namespace Medium
