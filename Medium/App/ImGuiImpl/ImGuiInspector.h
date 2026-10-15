///
/// @file   ImGuiInspector.h
/// @author Garfield949
/// @date   2026/6/21 19:48
/// @brief  
///
#pragma once

struct SDL_Window;

namespace Medium {

class WindowManager;
class SceneManager;
class RenderManager;
class FPSController;

class ImGuiInspector {
public:
  void run();

  FPSController* controller     = nullptr;
  WindowManager* window_manager = nullptr;
  SceneManager*  scene_manager  = nullptr;
  RenderManager* render_manager = nullptr;

private:
  float window_margin_ = 15.0f;
  
  float w_inspector_window_  = 400.0f;
  float w_statistics_window_ = 410.0f;
  float h_statistics_window_ = 0.0f;

private:
  bool show_csm_atlas_ = false; // 需保存为一个持久状态，确保其为 true 时不受 Inspector 或 Debug 面板折叠的影响。

private:
  void showStatisticsWindow();
  void showInspectorWindow();
  void showCSMAtlasWindow();
  
  void showScenePanel();
  void showGraphicsPanel();
  void showDebugPanel();
  void showFPSControllerInfoPanel();
  void showAboutPanel();
};

} // namespace Medium