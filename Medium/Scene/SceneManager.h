///
/// @file   SceneManager.h
/// @author Garfield949
/// @date   2026/6/21 16:27
/// @brief  
///
#pragma once

#include "Scene/Scene.h" // std::unique_ptr 需在编译期获取类型大小。

namespace Medium {

class SceneManager {
public:
  explicit SceneManager() = default;

  bool init(f32 aspect_ratio);

  /// @brief 切换场景
  /// @params lua_file_name 场景 lua 文件名。
  /// @params aspect_ratio 窗口宽高比，用于调整场景相机视椎。
  bool switchScene(std::string_view lua_file_name, f32 aspect_ratio);

  /// @brief 窗口大小变化回调，调整场景相机视椎。
  void onResize(const Vec2i& buffer_extent_physical) const;

  /// @brief 获取当前激活场景句柄。
  Scene* activeScene() const noexcept { return active_scene_.get(); }

private:
  // 使用智能指针的原因：若有场景切换需求，从 Scene::loadFromLua 接收一个 std::unique_ptr<Scene> 而无需深拷贝。
  std::unique_ptr<Scene> active_scene_ = nullptr;
};

} // Medium
