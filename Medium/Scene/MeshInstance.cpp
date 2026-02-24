///
/// @file   MeshInstance.cpp
/// @author Garfield949
/// @date   2026/2/13 22:58
/// @brief  
///
#include "MeshInstance.h"

#include "Camera.h"
#include "Pipeline/Rasterizer.h"
#include "Pipeline/Shader/IShader.h"
#include "Resources/Material.h"
#include "Resources/Mesh.h"

namespace Medium {

    void MeshInstance::render(RenderContext& context) {
        const Mat4f &w = transform.worldMatrix();
        const Mat4f &v = context.camera.viewMatrix();
        const Mat4f &p = context.camera.projMatrix();

        Uniform &uniform = context.shader.uniforms;
        uniform.world = w;
        uniform.wvp = p * v * w;
        uniform.camera_pos = context.camera.transform.worldPosition();

        // 遍历网格中的每一个子网格，发起多个 Draw Call
        for (const auto& submesh : mesh_->sub_meshes) {
            MaterialPtr mat = mesh_->materials[submesh.material_index];

            if(int mat_id = submesh.material_index; mat_id == -1) {
                context.shader.texture = nullptr;
                context.shader.uniforms.draw_checkerboard = true;
            } else {
                context.shader.texture = mat->map_Kd.get();
            }

            uniform.Kd = mat->Kd;

            // 调用光栅化器，绘制这个局部的 SubMesh
            context.rasterizer.draw(*(this->mesh_), submesh, context.shader, context.render_target);
        }

        Node::render(context);
    }

} // namespace Medium
