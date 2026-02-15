///
/// @file   MeshInstance.h
/// @author Garfield949
/// @date   2026/2/12 22:26
/// @brief  
///
#pragma once

#include "Scene/Node.h"
#include "Resources/Mesh.h"

namespace Medium {

    class MeshInstance : public Node {
    public:
        explicit MeshInstance(std::string name, std::shared_ptr<Mesh> mesh)
            : Node(std::move(name)), mesh_(std::move(mesh)) {}

        void render(RenderContext &context) override;

    private:
        std::shared_ptr<Mesh> mesh_;
    };

} // namespace Medium
