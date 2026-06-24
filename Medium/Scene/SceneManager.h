///
/// @file   SceneManager.h
/// @author Garfield949
/// @date   2026/6/21 16:27
/// @brief  
///
#pragma once

#include "Scene/Scene.h" // std::unique_ptr 需在编译期获取类型大小。

namespace Medium {

struct WindowCfg;

class SceneManager {
public:
  explicit SceneManager() = default;

  bool init(const WindowCfg& cfg_window);

  bool switchScene(std::string_view scene_name, f32 aspect_ratio);

  void onResize(const Vec2i& buffer_extent_physical) const;

  Scene* activeScene() const noexcept { return active_scene_.get(); }

private:
  // 使用智能指针的原因：若有场景切换需求，从 Scene::loadFromLua 接收一个 std::unique_ptr<Scene> 而无需深拷贝。
  std::unique_ptr<Scene> active_scene_ = nullptr;
};

} // Medium
