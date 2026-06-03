///
/// @file   Scene.cpp
/// @author Garfield949
/// @date   2026/3/17 21:27
/// @brief  
///
#include "Scene.h"

#include <fmt/base.h>

#include <lua/lua.hpp>
#include <LuaBridge.h>

#include "Scene/Node.h"
#include "Scene/Camera.h"
#include "Scene/Light/DirectionalLight.h"
#include "Scene/MeshNode.h"
#include "Pipeline/Shadow/CSM.h"
#include "Resources/AssetCache.h"

namespace {

using namespace Medium;

Vec3f readVec3f(const luabridge::LuaRef& tbl, std::string_view key) {
  luabridge::LuaRef v = tbl[key];
  return Vec3f{
    v[1].cast<f32>().value(),
    v[2].cast<f32>().value(),
    v[3].cast<f32>().value()
  };
}

}

// TODO: 鲁棒性处理（不过只要场景脚本写对也没啥大问题）
namespace Medium {

using namespace std;

struct SceneLoader {
  static void setupCamera(const luabridge::LuaRef& tbl_scene, Scene* scene, f32 aspect_ratio) {
    luabridge::LuaRef tbl_camera = tbl_scene["Camera"];
    f32 fov  = tbl_camera["fov"].cast<f32>().value();
    f32 near = tbl_camera["near"].cast<f32>().value();

    Vec3f pos = readVec3f(tbl_camera, "position");
    Vec3f rot = readVec3f(tbl_camera, "rotation");

    scene->main_camera_
      = scene->root_.attachChild(make_unique<Camera>("Main Camera", Camera::Mode::Perspective));
    scene->main_camera_->setPerspParams(fov, near, aspect_ratio);
    scene->main_camera_->transform.setPosition(pos);
    scene->main_camera_->transform.setRotation(rot);
  }

  static void setupLight(const luabridge::LuaRef& tbl_scene, Scene* scene) {
    luabridge::LuaRef tbl_dir = tbl_scene["Light"];
    Vec3f direction = readVec3f(tbl_dir, "direction");
    // 注意，初始化必须调用一次 lookAt 来调整方向光的姿态。每帧计算的 DirectionalView 仅是获取光相机的
    // 位置和正交范围，并不会改变光的朝向。
    scene->dir_light_ = scene->root_.attachChild(make_unique<DirectionalLight>("Main Light"));
    scene->dir_light_->dir = direction.normalized();
    scene->dir_light_->transform.lookAt(scene->dir_light_->dir);
  }

  static void readShadowCfg(const luabridge::LuaRef& tbl_scene, Scene* scene) {
    luabridge::LuaRef tbl_shadow_cfg  = tbl_scene["ShadowConfig"];
    scene->shadowCfg_.max_shadow_dist = tbl_shadow_cfg["max_shadow_distance"].cast<f32>().value();
    scene->shadowCfg_.csm_cascade_cnt
      = std::clamp(tbl_shadow_cfg["csm_cascade_count"].cast<u32>().value(), 1u, kMaxCSMCascades);
  }

  static bool setupModels(const luabridge::LuaRef& tbl_scene, Scene* scene) {
    luabridge::LuaRef tbl_model = tbl_scene["Model"];
    string name = tbl_model["name"];
    string path = tbl_model["file"];
    f32 scale   = tbl_model["scale"];

    luabridge::LuaRef tbl_pos = tbl_model["position"];
    Vec3f pos {
      tbl_pos[1].cast<f32>().value(),
      tbl_pos[2].cast<f32>().value(),
      tbl_pos[3].cast<f32>().value()
    };

    luabridge::LuaRef tbl_rot = tbl_model["rotation"];
    Vec3f rot {
      tbl_rot[1].cast<f32>().value(),
      tbl_rot[2].cast<f32>().value(),
      tbl_rot[3].cast<f32>().value()
    };

    auto obj_path = filesystem::path(PROJECT_ROOT_DIR) / path;

    MeshPtr mesh = AssetCache::instance().loadFromObj(obj_path);
    if(!mesh) return false;

    auto model = scene->root_.attachChild(make_unique<MeshNode>(name, mesh));
    model->transform.setPosition(pos);
    model->transform.setRotation(rot);
    model->transform.setScale(scale);
    scene->mesh_nodes_.push_back(model);

    return true;
  }
};

std::unique_ptr<Scene> Scene::loadFromLua(const std::filesystem::path& lua_path, f32 aspect_ratio) {
  std::unique_ptr<Scene> scene{new Scene};

  lua_State* L = luaL_newstate();
  luaL_openlibs(L);

  if(luaL_dofile(L, lua_path.generic_string().c_str()) != LUA_OK) {
    fmt::print(stderr, "\nScene::loadFromLua - Lua::{}\n", lua_tostring(L, -1));
    return nullptr;
  }

  // LuaRef 是一种任意值类型
  luabridge::LuaRef tbl_scene = luabridge::LuaRef::fromStack(L, -1);
  
  SceneLoader::setupCamera(tbl_scene, scene.get(), aspect_ratio);
  SceneLoader::setupLight (tbl_scene, scene.get());
  SceneLoader::readShadowCfg(tbl_scene, scene.get());
  if(!SceneLoader::setupModels(tbl_scene, scene.get()))
    return nullptr;

  return scene;
}

AABB3D Scene::worldAABB() const noexcept {
  AABB3D bbox_world;
  // 注：由于该方法是 const 的，故形参得为 const.
  root_.traverse([&](const Node* node) {
    if(const auto* mesh_node = Node::cast_to<MeshNode>(node)) {
      bbox_world.merge(mesh_node->worldAABB());
    }
  });
  return bbox_world;
}

} // namespace Medium