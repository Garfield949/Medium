///
/// @file   SceneManager.cpp
/// @author Garfield949
/// @date   2026/6/21 16:27
/// @brief  
///
#include "SceneManager.h"

#include <filesystem>

#include "Base/Log.h"
#include "Scene/Camera.h"

namespace Medium {

bool SceneManager::init(f32 aspect_ratio) {
  return switchScene("sponza.lua", aspect_ratio);
}

bool SceneManager::switchScene(std::string_view lua_file_name, f32 aspect_ratio) {
  auto scene_lua = std::filesystem::path(PROJECT_ROOT_DIR) / "scenes"
    / lua_file_name;

  auto new_scene = Scene::LoadFromLua(scene_lua, aspect_ratio);
  if(!new_scene) {
    Log::printLogPrefix("SceneManager", "switchScene", true);
    fmt::print("Failed\n");
    return false;
  }

  if(!new_scene->switchSkybox("desert.png"))
    return false;
  
  // 这里会调用 unique_ptr 的移动赋值，并自动释放掉之前的场景资源。
  active_scene_ = std::move(new_scene);
  return true;
}

void SceneManager::onResize(const Vec2i& buffer_extent_physical) const {
  // 注：更改 aspect ratio 后，相机视椎形状会变，进而使阴影拟合产生变化
  active_scene_->mainCamera()->setAspectRatio(
    static_cast<f32>(buffer_extent_physical.x()) / static_cast<f32>(buffer_extent_physical.y())
  );
}

} // Medium