///
/// @file   DrawQueue.cpp
/// @author Garfield949
/// @date   2026/4/3 15:24
/// @brief  
///
#include "DrawQueue.h"

#include <algorithm>
#include <functional>

#include "Scene/MeshNode.h"
#include "Resources/Mesh.h"
#include "Resources/Material.h"
#include "Pipeline/RenderContext.h"
#include "Pipeline/DrawCommand.h"

namespace Medium {

void DrawQueue::build(std::span<const MeshNode* const> mesh_nodes, const CameraContext& cam_ctx) {
  for(auto mesh_node: mesh_nodes) {
    const Mesh*    mesh    = mesh_node->mesh();
    const Mat4f&   world   = mesh_node->transform.worldMatrix();
    const Frustum& frustum = cam_ctx.frustum;

    // 在 Mesh 层面进行视椎剔除。
    if(frustum.intersects(mesh_node->localAABB().toWorldAABB(world))) {
      for(const auto& submesh: mesh->sub_meshes) {
        const Material& submesh_material = mesh->materials[submesh.material_index];
        // 根据子网格绑定的材质类型决定是否纳入队列。
        if(this->queue_type_ == Type::Opaque && 
           submesh_material.surface_mode == Material::SurfaceMode::Transparent) continue;
        if(this->queue_type_ == Type::Transparent &&
           submesh_material.surface_mode != Material::SurfaceMode::Transparent) continue;

        // 在 SubMesh 层面进行视椎剔除。
        AABB3D bbox_world = submesh.aabb_local.toWorldAABB(world);
        if(!frustum.intersects(bbox_world)) continue;

        // 将每个 SubMesh 的包围盒角点变换到相机空间，并取角点中最大 z 作为 sort key; 排序时按 max_z 从
        // 小到大进行排序(见下方 sort())。这个策略在场景物体在相机视角里远近跨度很大时能有效抑制 overdraw.
        // 原理：当场景物体在相机视角里远近跨度很大时，取包围盒角点 min_z 反而会加剧 overdraw，因为这样的
        // 物体远端均离相机很远，然而近端越靠近相机其覆盖的屏幕像素只会更多。这样的物体若本该被其它物体遮挡，但
        // 却被排在队列前面的话，就会产生 overdraw. 取 max_z 作为 sort key 在这样的情景下相对更能反应物体
        // 的跨度。
        f32 max_z_view = std::numeric_limits<f32>::lowest();
        for(auto& corner: bbox_world.corners())
          max_z_view = std::max(max_z_view, (cam_ctx.mat_view * corner.homogeneous()).z());

        DrawCommand cmd {
          .mesh      = mesh,
          .submesh   = &submesh,
          .material  = &submesh_material,
          .mat_world = world,
          .sort_key  = max_z_view
        };

        cmds_.push_back(cmd);
      }
    }
  }
}

void DrawQueue::sort() {
  switch(queue_type_) {
  case Type::Opaque: sortOpaque(); break;
  case Type::Transparent: sortTransparent(); break;
  }
}

void DrawQueue::sortOpaque() {
  // &Class::field 指向类 Class 中成员变量 field 的指针；参数二是比较器，{} 默认构造一个 std::ranges::less{}.
  std::ranges::sort(cmds_, {}, &DrawCommand::sort_key);
}

void DrawQueue::sortTransparent() {
  std::ranges::sort(cmds_, std::ranges::greater{}, &DrawCommand::sort_key);
}

} // namespace Medium
