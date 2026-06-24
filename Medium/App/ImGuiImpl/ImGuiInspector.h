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
  void show();

  FPSController* controller     = nullptr;
  WindowManager* window_manager = nullptr;
  SceneManager*  scene_manager  = nullptr;
  RenderManager* render_manager = nullptr;

private:
  float w_inspector = 410;
  
  void showBasicInfoPanel();
  void showDebugPanel();
  void showSceneInfoPanel();
  void showCameraInfoPanel();
  void showFPSControllerInfoPanel();
  void showAboutPanel();
};

} // namespace Medium