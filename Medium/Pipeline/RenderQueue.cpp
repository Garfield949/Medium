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

void RenderQueue::build(const Node* scene_root, const ViewCtx& view_ctx) {
  scene_root->traverse([&](const Node* node) {
    if(const auto* mesh_node = Node::cast_to<MeshNode>(node)) {
      const MeshPtr& mesh    = mesh_node->mesh();
      const Mat4f&   world   = mesh_node->transform.worldMatrix();
      const Frustum& frustum = view_ctx.frustum;
      
      if(frustum.intersects(mesh_node->aabb().toWorldAABB(world))) {
        for(const auto& sub_mesh : mesh->sub_meshes) {
          AABB3D bbox_world = sub_mesh.bbox.toWorldAABB(world);
          if(!frustum.intersects(bbox_world)) continue;
          cmds_.emplace_back(
            mesh.get(),
            &sub_mesh,
            mesh->materials[sub_mesh.material_index].get(),
            &world,
            (view_ctx.view * bbox_world.center().homogeneous()).z()
          );
        }
      }
    }
  });
}

void RenderQueue::sort() {
  switch(type_) {
    case Type::Shadow: sortShadow(); break;
    case Type::Opaque: sortOpaque(); break;
    case Type::Transparent: sortTransparent(); break;
  }
}

void RenderQueue::sortShadow() {
  std::sort(cmds_.begin(), cmds_.end(), [](const RenderCommand& a, const RenderCommand& b) {
    return a.sort_key < b.sort_key;
  });
}

void RenderQueue::sortOpaque() {
  std::sort(cmds_.begin(), cmds_.end(), [](const RenderCommand& a, const RenderCommand& b) {
    return a.sort_key > b.sort_key;
  });
}

void RenderQueue::sortTransparent() {

}

} // namespace Medium
