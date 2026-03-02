///
/// @file   MeshInstance.cpp
/// @author Garfield949
/// @date   2026/2/13 22:58
/// @brief
///
#include "MeshInstance.h"

#include "Base/Types.h"
#include "Pipeline/Shader/Shaders.h"
#include "Resources/Material.h"
#include "Scene/Camera.h"
#include "Resources/Mesh.h"
#include "Resources/Texture.h"
#include "Pipeline/Pipeline.h"
#include "Pipeline/Shader/ShaderType.h"

namespace Medium {

    void MeshInstance::render(RenderContext& context) {

        Uniform& uniforms = std::visit(overloads {
            [&](BlinnPhongShader& s) -> Uniform& {
                Uniform& u = s.uniforms;
                Mat4f world = transform.worldMatrix();
                u.world = world;
                u.wvp = context.projection_view * world;
                u.camera_pos = context.camera.transform.worldPosition();
                u.light_dir = Vec3f(3.0f, 2.0f, -1.0f).normalized();
                return u;
            }
        }, context.shader);

        for (const auto& submesh : mesh_->sub_meshes) {
            int curr_mat_idx = submesh.material_index;
            // 为当前 SubMesh 绑定材质参数
            if (curr_mat_idx == -1) {
                uniforms.color_source_diffuse  = ColorSource::Checkerboard;
                uniforms.color_source_specular = ColorSource::Checkerboard;
                uniforms.map_Kd     = nullptr;
                uniforms.map_Ks     = nullptr;
                uniforms.mat_params = MaterialParams{};
            } else {
                // Lambda.
                auto inferColorSource = [&](const Texture* tex) -> ColorSource {
                    if(tex) return ColorSource::Textured;
                    return ColorSource::SolidColor;
                };

                uniforms.map_Kd = mesh_->materials[curr_mat_idx]->map_Kd.get();
                uniforms.map_Ks = mesh_->materials[curr_mat_idx]->map_Ks.get();

                uniforms.color_source_diffuse  = inferColorSource(uniforms.map_Kd);
                uniforms.color_source_specular = inferColorSource(uniforms.map_Ks);
                
                uniforms.mat_params = mesh_->materials[curr_mat_idx]->params;
            }

            // 各 SubMesh 一次 Draw Call.
            Pipeline::draw(*(this->mesh_), submesh, context.shader, context.render_target);
        }

        Node::render(context);
    }

} // namespace Medium
