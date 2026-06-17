///
/// @file   WindowManager.h
/// @author Garfield949
/// @date   2026/6/14 21:38
/// @brief  
///
#pragma once

#include "Base/Types.h"

struct SDL_Window;    // 窗口句柄；
struct SDL_Renderer;  // 2D 绘制句柄，与一个 SDL_Window 关联；
struct SDL_Texture;   // 可被绘制的图像缓冲，与一个 SDL_Renderer 绑定。

namespace Medium {

struct WindowCfg;
  
class WindowManager {
public:
  explicit WindowManager() = default;
  ~WindowManager();

  bool init(const WindowCfg& cfg_window);

  std::pair<i32, i32> physicalExtent() const;

  bool onResize();

  void commitBuffer(const u32* packed_abgr8_buffer, u32 buffer_width, u32 buffer_height);

private:
  bool createSDLWindow(const WindowCfg& cfg_window);
  bool createSDLRenderer();
  bool createSDLTexture();

  SDL_Window*   sdl_window_          = nullptr;
  SDL_Renderer* sdl_renderer_        = nullptr;
  SDL_Texture*  sdl_texture_present_ = nullptr;
};

} // namespaceMedium
