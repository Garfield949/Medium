///
/// @file   MeshInstance.h
/// @author Garfield949
/// @date   2026/2/12 22:26
/// @brief  
///
#pragma once

#include "Scene/Node.h"
#include "Resources/Mesh.h"
#include "Resources/Texture.h"

namespace Medium {

    class MeshInstance : public Node {
    public:
        explicit MeshInstance(std::string name, std::shared_ptr<Mesh> mesh)
            : Node(std::move(name)), mesh_(std::move(mesh)) {}

        void render(RenderContext &context) override;

        void setTexture(std::shared_ptr<Texture> texture);

    private:
        std::shared_ptr<Mesh> mesh_;
        std::shared_ptr<Texture> texture_;
    };

} // namespace Medium
