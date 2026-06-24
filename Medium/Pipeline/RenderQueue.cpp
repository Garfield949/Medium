///
/// @file   RenderQueue.cpp
/// @author Garfield949
/// @date   2026/4/3 15:24
/// @brief  
///
#include "RenderQueue.h"

#include "Base/Types.h"
#include "Scene/MeshNode.h"
#include "Resources/Mesh.h"
#include "Resources/Material.h"
#include "Pipeline/RenderContext.h"
#include "Pipeline/RenderCommand.h"

namespace Medium {

void RenderQueue::build(std::span<const MeshNode* const> mesh_nodes, const CameraCtx& view_ctx) {
  for(auto mesh_node: mesh_nodes) {
    const MeshPtr& mesh    = mesh_node->mesh();
    const Mat4f&   world   = mesh_node->transform.worldMatrix();
    const Frustum& frustum = view_ctx.frustum;

    if(frustum.intersects(mesh_node->localAABB().toWorldAABB(world))) {
      for(const auto& sub_mesh: mesh->sub_meshes) {
        
        const Material* material = mesh->materials[sub_mesh.material_index].get();

        // 根据子网格绑定的材质类型决定是否纳入队列。
        if(type_ == Type::OpaqueQueue && 
          material->surface_mode == Material::SurfaceMode::Transparent) continue;
        if(type_ == Type::TransparentQueue && 
          material->surface_mode != Material::SurfaceMode::Transparent) continue;
        
        AABB3D bbox_world = sub_mesh.aabb_local.toWorldAABB(world);
        if(!frustum.intersects(bbox_world)) continue;

        f32 max_z_view = std::numeric_limits<f32>::lowest();
        for(auto& corner: bbox_world.corners())
          max_z_view = std::max(max_z_view, (view_ctx.view * corner.homogeneous()).z());

        cmds_.emplace_back(
          mesh.get(),
          &sub_mesh,
          material,
          world,
          max_z_view
        );
      }
    }
  }
}

void RenderQueue::sort() {
  switch(type_) {
  case Type::OpaqueQueue: sortOpaque(); break;
  case Type::TransparentQueue: sortTransparent(); break;
  }
}

void RenderQueue::sortOpaque() {
  std::sort(cmds_.begin(), cmds_.end(), [](const RenderCommand& a, const RenderCommand& b) {
    return a.sort_key < b.sort_key;
  });
}

void RenderQueue::sortTransparent() {
  std::sort(cmds_.begin(), cmds_.end(), [](const RenderCommand& a, const RenderCommand& b) {
    return a.sort_key > b.sort_key;
  });
}

} // namespace Medium
