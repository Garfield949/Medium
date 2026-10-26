///
/// @file   Scene.cpp
/// @author Garfield949
/// @date   2026/3/17 21:27
/// @brief  
///
#include "Scene.h"

#include <fmt/base.h>
#include <fmt/color.h>

#include <lua/lua.hpp>
#include <LuaBridge.h>

#include "Base/Log.h"
#include "Scene/Node.h"
#include "Scene/Camera.h"
#include "Scene/Light/DirectionalLight.h"
#include "Scene/MeshNode.h"
#include "Pipeline/Shadow/CSM.h"
#include "Resources/AssetCache.h"

namespace Medium {
namespace {

Vec3f readVec3f(const luabridge::LuaRef& tbl, std::string_view key) {
  luabridge::LuaRef v = tbl[key];
  return Vec3f{
    v[1].cast<f32>().value(),
    v[2].cast<f32>().value(),
    v[3].cast<f32>().value()
  };
}

} // 匿名命名空间，控制仅单文件可见
} // namespace Medium


// TODO: 鲁棒性处理（不过只要场景脚本写对也没啥大问题）
namespace Medium {

using namespace std;

struct SceneLoader {
  static void SetupCamera(const luabridge::LuaRef& tbl_scene, Scene* scene, f32 aspect_ratio) {
    luabridge::LuaRef tbl_camera = tbl_scene["Camera"];
    f32 fov  = tbl_camera["fov"].cast<f32>().value();

    Vec3f pos = readVec3f(tbl_camera, "position");
    Vec3f rot = readVec3f(tbl_camera, "rotation");

    scene->main_camera_ = scene->root_.attachChild(make_unique<Camera>("Main Camera", Camera::Mode::Perspective));
    scene->main_camera_->setPerspParams(fov, aspect_ratio);
    scene->main_camera_->transform.setPosition(pos);
    scene->main_camera_->transform.setRotation(rot);
  }

  static void SetupLight(const luabridge::LuaRef& tbl_scene, Scene* scene) {
    luabridge::LuaRef tbl_dir = tbl_scene["Light"];
    Vec3f direction = readVec3f(tbl_dir, "direction");
    // 注意，初始化必须调用一次 setDir 来调整方向光的姿态。每帧计算的 DirectionalView 仅是获取并调整
    // 光相机的位置和正交范围，并不会改变光的朝向。
    scene->dir_light_ = scene->root_.attachChild(make_unique<DirectionalLight>("Main Light"));
    scene->dir_light_->setDir(direction);
  }

  static void ReadShadowConfig(const luabridge::LuaRef& tbl_scene, Scene* scene) {
    luabridge::LuaRef tbl_shadow_cfg  = tbl_scene["ShadowConfig"];
    scene->shadow_cfg_.max_shadow_dist = tbl_shadow_cfg["max_shadow_distance"].cast<f32>().value();
    scene->shadow_cfg_.csm_cascade_cnt
      = std::clamp(tbl_shadow_cfg["csm_cascade_count"].cast<i32>().value(), kMinCSMCascades, kMaxCSMCascades);
  }

  static bool SetupModels(const luabridge::LuaRef& tbl_scene, Scene* scene) {
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

    const Mesh* mesh = AssetCache::instance().loadFromObj(obj_path);
    if(!mesh) return false;

    auto model = scene->root_.attachChild(make_unique<MeshNode>(name, mesh));
    model->transform.setPosition(pos);
    model->transform.setRotation(rot);
    model->transform.setScale(scale);
    scene->mesh_nodes_.push_back(model);

    return true;
  }
};

std::unique_ptr<Scene> Scene::LoadFromLua(const std::filesystem::path& lua_path, f32 aspect_ratio) {
  std::unique_ptr<Scene> scene = std::make_unique<Scene>();

  lua_State* L = luaL_newstate();
  luaL_openlibs(L);

  if(luaL_dofile(L, lua_path.generic_string().c_str()) != LUA_OK) {
    Log::printLogPrefix("Scene", "LoadFromLua", true);
    fmt::print("Lua::{}\n", lua_tostring(L, -1));
    lua_close(L);
    return nullptr;
  }

  { // 注：LuaRef 本身依赖 lua_State，若不加一个局部作用域的话， tbl_scene 只会在 return scene 后被析构，而
    // 在 return 之前已调用过一次 lua_close，此时 tbl_scene 持有的 lua_State 已被释放，再次析构会导致 UB.
    // LuaRef 是一种任意值类型
    luabridge::LuaRef tbl_scene = luabridge::LuaRef::fromStack(L, -1);
    
    SceneLoader::SetupCamera(tbl_scene, scene.get(), aspect_ratio);
    SceneLoader::SetupLight (tbl_scene, scene.get());
    SceneLoader::ReadShadowConfig(tbl_scene, scene.get());
    if(!SceneLoader::SetupModels(tbl_scene, scene.get())) {
      lua_close(L);
      return nullptr;
    }
  }

  lua_close(L);

  return scene;
}

bool Scene::switchSkybox(std::string_view cubemap_img_filename) {
  auto path = std::filesystem::path(PROJECT_ROOT_DIR) / "assets" / "skyboxes"
    / cubemap_img_filename;

  auto skybox = AssetCache::instance().loadSkyBox(path);

  if(!skybox) {
    Log::printLogPrefix("SceneManager", "switchSkybox", true);
    fmt::print("Failed\n");
    return false;
  }

  skybox_ = skybox;
  return true;
}

AABB3D Scene::worldAABB() const noexcept {
  AABB3D bbox_world;
  // 注：由于该方法是 const 的，故形参得为 const.
  root_.traverse([&](const Node* node) {
    if(const auto* mesh_node = Node::CastTo<MeshNode>(node)) {
      bbox_world.merge(mesh_node->worldAABB());
    }
  });
  return bbox_world;
}

} // namespace Medium