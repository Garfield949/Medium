///
/// @file   Scene.cpp
/// @author Garfield949
/// @date   2026/3/17 21:27
/// @brief  
///
#include "Scene.h"

#include <fmt/base.h>
#include <sol/sol.hpp>

#include "App/Config.h"
#include "Scene/Node.h"
#include "Scene/Camera.h"
#include "Scene/Light/DirectionalLight.h"
#include "Scene/MeshNode.h"
#include "Resources/AssetCache.h"

namespace Medium {

  struct SceneLoader {
    static void setupCamera(const sol::table& lua_scene, Scene* scene, f32 aspect_ratio) {
      using namespace std;
      sol::table lua_cam = lua_scene["Camera"];
      f32    fov  = lua_cam["fov"];
      f32    near = lua_cam["near"];
      Vec3f  pos  = Eigen::Map<Vec3f>(lua_cam["position"].get<std::vector<f32>>().data());
      Vec3f  rot  = Eigen::Map<Vec3f>(lua_cam["rotation"].get<std::vector<f32>>().data());

      scene->main_camera_ = scene->root_.attachChild(make_unique<Camera>("Main Camera", Camera::Mode::Persp));
      scene->main_camera_->setPerspParams(fov, near, aspect_ratio);
      scene->main_camera_->transform.setPosition(pos);
      scene->main_camera_->transform.setRotation(rot);
    }
    static void setupLight(const sol::table& lua_scene, Scene* scene) {
      using namespace std;
      sol::table lua_light = lua_scene["Light"];
      Vec3f direction = Eigen::Map<Vec3f>(lua_light["direction"].get<vector<f32>>().data());

      scene->main_light_ = scene->root_.attachChild(make_unique<DirectionalLight>("Main Light"));
      scene->main_light_->dir = direction.normalized();
    }
    static bool setupModels(const sol::table& lua_scene, Scene* scene) {
      using namespace std;
      sol::table lua_model = lua_scene["Model"];
      string name = lua_model["name"];
      string path = lua_model["file"];
      f32 scale   = lua_model["scale"];
      Vec3f pos   = Eigen::Map<Vec3f>(lua_model["position"].get<vector<f32>>().data());
      Vec3f rot   = Eigen::Map<Vec3f>(lua_model["rotation"].get<vector<f32>>().data());

      auto obj_path = filesystem::path(PROJECT_ROOT_DIR) / path;

      MeshPtr mesh = AssetCache::Instance().loadFromObj(obj_path);
      if(!mesh) return false;

      auto model = scene->root_.attachChild(make_unique<MeshNode>(name, mesh));
      model->transform.setPosition(pos);
      model->transform.setRotation(rot);
      model->transform.setScale(scale);

      return true;
    }
  };

  std::unique_ptr<Scene> Scene::loadFromLua(const std::filesystem::path& lua_path, f32 aspect_ratio) {
    std::unique_ptr<Scene> scene{new Scene};

    sol::state lua;
    lua.open_libraries(sol::lib::base);

    sol::table lua_scene = lua.script_file(lua_path.generic_string());
    
    SceneLoader::setupCamera(lua_scene, scene.get(), aspect_ratio);
    SceneLoader::setupLight (lua_scene, scene.get());
    if(!SceneLoader::setupModels(lua_scene, scene.get()))
      return nullptr;

    return scene;
  }

} // namespace Medium