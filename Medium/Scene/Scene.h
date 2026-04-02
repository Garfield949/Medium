///
/// @file   Scene.h
/// @author Garfield949
/// @date   2026/3/17 21:27
/// @brief  
///
#pragma once

#include <filesystem>

#include "Scene/Node.h"

namespace Medium {

  class Camera;
  class DirectionalLight;

  class Scene {
    friend struct SceneLoader;
  public:
    [[nodiscard]] 
    static std::unique_ptr<Scene> loadFromLua(const std::filesystem::path& lua_path, f32 aspect_ratio);

    [[nodiscard]] Node* root() noexcept { return &root_; }
    [[nodiscard]] Camera* mainCam() const noexcept { return main_camera_; }
    [[nodiscard]] DirectionalLight* mainLight() const noexcept { return main_light_; }
  
  private:
    Scene() = default;

    Node root_ = {"Scene Root"};

    Camera* main_camera_ = nullptr;
    
    DirectionalLight* main_light_  = nullptr;
  };

} // Medium