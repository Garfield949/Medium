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

namespace Medium {

    void MeshInstance::render(RenderContext& context) {
        const Mat4f &w = transform.worldMatrix();
        const Mat4f &v = context.camera.viewMatrix();
        const Mat4f &p = context.camera.projMatrix();

        Uniform &uniform = context.shader.uniforms;
        uniform.world = w;
        uniform.wvp = p * v * w;
        uniform.camera_pos = context.camera.transform.worldPosition();

        context.shader.texture = texture_.get();

        context.rasterizer.draw(*(this->mesh_),
                             context.shader,
                             context.render_target);
        Node::render(context);
    }

    void MeshInstance::setTexture(std::shared_ptr<Texture> texture) {
        texture_ = std::move(texture);
    }

} // namespace Medium
