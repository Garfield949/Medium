///
/// @file   MeshInstance.cpp
/// @author Garfield949
/// @date   2026/2/13 22:58
/// @brief
///
#include "MeshInstance.h"

#include "Base/Types.h"
#include "Resources/Texture.h"
#include "Scene/Camera.h"
#include "Pipeline/Rasterizer.h"
#include "Pipeline/Shader/IShader.h"
#include "Resources/Mesh.h"

namespace Medium {

    void MeshInstance::render(RenderContext& context) {
        // w 和 v 返回的是临时对象，所以不使用引用。
        const Mat4f w  = transform.worldMatrix();
        const Mat4f v  = context.camera.viewMatrix();
        const Mat4f &p = context.camera.projMatrix();

        Uniform &uniforms = context.shader.uniforms;
        uniforms.world = w;
        uniforms.wvp = p * v * w;
        uniforms.camera_pos = context.camera.transform.worldPosition();
        uniforms.light_dir = {1.0f, 1.0f, -1.0f};
        uniforms.light_dir.normalize();

        for (const auto& submesh : mesh_->sub_meshes) {
            int curr_mat_idx = submesh.material_index;
            // 为当前 SubMesh 绑定材质参数
            if (curr_mat_idx == -1) {
                uniforms.color_source_diffuse = ColorSource::Checkerboard;
            } else {
                // Lambda.
                auto inferColorSource = [&](const Texture* tex) -> ColorSource {
                    if(tex) return ColorSource::Textured;
                    else    return ColorSource::SolidColor;
                };

                uniforms.map_Kd = mesh_->materials[curr_mat_idx]->map_Kd.get();
                uniforms.map_Ks = mesh_->materials[curr_mat_idx]->map_Ks.get();

                uniforms.color_source_diffuse  = inferColorSource(uniforms.map_Kd);
                uniforms.color_source_specular = inferColorSource(uniforms.map_Ks);
                
                uniforms.mat_params = mesh_->materials[curr_mat_idx]->params;
            }

            // 各 SubMesh 一次 Draw Call.
            context.rasterizer.draw(*(this->mesh_), submesh, context.shader, context.render_target);
        }

        Node::render(context);
    }

} // namespace Medium
