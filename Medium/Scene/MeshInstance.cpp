///
/// @file   MeshInstance.cpp
/// @author Garfield949
/// @date   2026/2/13 22:58
/// @brief
///
#include "MeshInstance.h"

#include "Pipeline/RenderCommand.h"
#include "Pipeline/RenderQueue.h"
#include "Resources/Mesh.h"
#include "Geometry/Frustum.h"

namespace Medium {
  
  void MeshInstance::commitCommand(RenderQueue& queue, const Frustum& frustum) const {
    const Mat4f& mat_wld = transform.worldMatrix();
    
    if(frustum.intersects(mesh_->bbox.toWorldAABB(mat_wld))) {
      for(const auto& sub_mesh : mesh_->sub_meshes) {
        if(!frustum.intersects(sub_mesh.bbox.toWorldAABB(mat_wld)))
          continue;
        queue.collectCommands({
          mesh_.get(), 
          &sub_mesh,
          mesh_->materials[sub_mesh.material_index].get(),
          &mat_wld,
        });
      }
    }
    
    Node::commitCommand(queue, frustum);
  }

} // namespace Medium
