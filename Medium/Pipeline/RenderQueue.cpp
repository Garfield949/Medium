///
/// @file   RenderQueue.cpp
/// @author Garfield949
/// @date   2026/4/3 15:24
/// @brief  
///
#include "RenderQueue.h"

#include "Scene/MeshNode.h"
#include "Resources/Mesh.h"
#include "Pipeline/RenderContext.h"

namespace Medium {

void RenderQueue::build(std::span<const MeshNode* const> mesh_nodes, const CameraCtx& view_ctx) {
  for(auto mesh_node: mesh_nodes) {
    const MeshPtr& mesh    = mesh_node->mesh();
    const Mat4f&   world   = mesh_node->transform.worldMatrix();
    const Frustum& frustum = view_ctx.frustum;

    if(frustum.intersects(mesh_node->localAABB().toWorldAABB(world))) {
      for(const auto& sub_mesh: mesh->sub_meshes) {
        AABB3D bbox_world = sub_mesh.bbox.toWorldAABB(world);
        if(!frustum.intersects(bbox_world)) continue;
        cmds_.emplace_back(
          mesh.get(),
          &sub_mesh,
          mesh->materials[sub_mesh.material_index].get(),
          world,
          (view_ctx.view * bbox_world.center().homogeneous()).z()
        );
      }
    }
  }
}

void RenderQueue::sort() {
  switch(type_) {
    case Type::Shadow: sortShadow(); break;
    case Type::Opaque: sortOpaque(); break;
    case Type::Transparent: sortTransparent(); break;
  }
}

void RenderQueue::sortShadow() {
  // TODO: 配置一个合理的 sort key.
  std::sort(cmds_.begin(), cmds_.end(), [](const RenderCommand& a, const RenderCommand& b) {
    return a.sort_key < b.sort_key;
  });
}

void RenderQueue::sortOpaque() {
  // TODO: 配置一个合理的 sort key.
  std::sort(cmds_.begin(), cmds_.end(), [](const RenderCommand& a, const RenderCommand& b) {
    return a.sort_key > b.sort_key;
  });
}

void RenderQueue::sortTransparent() {

}

} // namespace Medium
