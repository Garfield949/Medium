///
/// @file   Scene.h
/// @author Garfield949
/// @date   2026/3/17 21:27
/// @brief  
///
#pragma once

#include <filesystem>
#include <span>

#include "Base/Geometry/AABB3D.h"
#include "Scene/Node.h"

namespace Medium {

class Camera;
class DirectionalLight;
class MeshNode;

class Scene {
  /// @brief 从 .obj 构建场景时，为了方便需访问 Scene 实例的私有成员，故将 SceneLoader 作为友元。
  friend struct SceneLoader;
public:
  struct ShadowConfig {
    f32 max_shadow_dist = 0.0f;
    i32 csm_cascade_cnt = 0;
  };

  explicit Scene() = default;

  [[nodiscard]]
  static std::unique_ptr<Scene> LoadFromLua(const std::filesystem::path& lua_path, f32 aspect_ratio);

  auto root(this auto& self) noexcept {
    return &self.root_;
  }

  auto mainCamera(this auto& self) noexcept {
    return self.main_camera_;
  }
  
  DirectionalLight* directionalLight() const noexcept { return dir_light_; }

  // 注：该方法的 const 修饰符，隐含了返回类型 const std::vector<const MeshNode*> : 即既无法改变数组中
  // MeshNode* 指向的内容，也禁止改变 MeshNode* 本身（指向别的地址）。故 span 的模板参数必须为一个同时为
  // 顶层和低层 const 的指针。
  std::span<const MeshNode* const> meshNodes() const noexcept { return mesh_nodes_; }

  /// @brief 返回场景的世界空间包围盒。
  AABB3D worldAABB() const noexcept;

  /// @brief 返回场景 .lua 的阴影配置
  ShadowConfig shadowConfig() const noexcept { return shadow_cfg_; }

private:
  Node root_{"Scene Root"};

  // 相机和方向光是观察指针，实例的生命周期由 root_ 持有，root_ 析构时会自动释放。
  Camera* main_camera_ = nullptr;  
  DirectionalLight* dir_light_ = nullptr;

  std::vector<MeshNode*> mesh_nodes_;

  ShadowConfig shadow_cfg_;
};

} // namespace Medium