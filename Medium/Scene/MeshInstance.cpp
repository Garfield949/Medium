///
/// @file   MeshInstance.cpp
/// @author Garfield949
/// @date   2026/2/13 22:58
/// @brief
///
#include "MeshInstance.h"

#include "Base/Meta.h"
#include "Pipeline/Pipeline.h"
#include "Pipeline/RenderContext.h"
#include "Pipeline/Shader/ShadowShader.h"
#include "Resources/Mesh.h"

namespace Medium {

  void MeshInstance::render(const RenderContext& context) {

    Mat4f world = this->transform.worldMatrix();
    
    std::visit(overloads {
      
      [&](BlinnPhongShader& s) {
        for (const auto& submesh : mesh_->sub_meshes) {
          int mat_idx = submesh.material_index;
          s.bind(context, world, mesh_->materials[mat_idx].get());
          // 各 SubMesh 一次 Draw Call.
          Pipeline::draw(*(this->mesh_), submesh, context.shader, context.buffer);
        }
      },
      [&](ShadowShader& s) {
        for (const auto& submesh : mesh_->sub_meshes) {
          int mat_idx = submesh.material_index;
          s.bind(context, world, mesh_->materials[mat_idx].get());
          // 各 SubMesh 一次 Draw Call.
          Pipeline::draw(*(this->mesh_), submesh, context.shader, context.buffer);
        }
      }
      
    }, *context.shader);

    Node::render(context);
  }

} // namespace Medium
