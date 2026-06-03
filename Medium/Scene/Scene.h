///
/// @file   Scene.h
/// @author Garfield949
/// @date   2026/3/17 21:27
/// @brief  
///
#pragma once

#include <filesystem>
#include <span>

#include "Scene/Node.h"
#include "Base/Geometry/AABB3D.h"

namespace Medium {

class Camera;
class DirectionalLight;
class MeshNode;

class Scene {
  /// @brief 构建场景时，为了方便需访问 Scene 实例的私有成员，故将 SceneLoader 作为友元。
  friend struct SceneLoader;
public:
  struct ShadowCfg {
    f32 max_shadow_dist;
    u32 csm_cascade_cnt;
  };

  [[nodiscard]]
  static std::unique_ptr<Scene> loadFromLua(const std::filesystem::path& lua_path, f32 aspect_ratio);

  Node* root() noexcept { return &root_; }
  const Node* root() const noexcept { return &root_; }

  // 注意：区分重载函数时，完全无视返回值类型。两个 camera 方法只能有一个标 const，因为它修饰的是 this:
  // Scene* s = ...;       s->camera(); // this = Scene*
  // const Scene* cs = s; cs->camera(); // this = const Scene*
  Camera* camera() noexcept { return main_camera_; }
  const Camera* camera() const noexcept { return main_camera_; }
  
  DirectionalLight* dirLight() const noexcept { return dir_light_; }

  // 注：该方法的 const 修饰符，隐含了返回类型 const std::vector<const MeshNode*> : 即既无法改变数组中
  // MeshNode* 指向的内容，也禁止改变 MeshNode* 本身（指向别的地址）。故 span 的模板参数必须为一个同时为
  // 顶层和低层 const 的指针。
  std::span<const MeshNode* const> meshNodes() const noexcept { return mesh_nodes_; }

  /// @brief 返回场景的世界空间包围盒。
  AABB3D worldAABB() const noexcept;

  /// @brief 返回场景的阴影配置
  ShadowCfg shadowCfg() const noexcept { return shadowCfg_; }

private:
  Scene() = default;

  Node root_ = {"Scene Root"};

  Camera* main_camera_ = nullptr;
  
  DirectionalLight* dir_light_ = nullptr;

  std::vector<MeshNode*> mesh_nodes_;

  ShadowCfg shadowCfg_;
};

} // namespace Medium