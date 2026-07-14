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
  
class WindowManager {
public:
  explicit WindowManager() = default;
  ~WindowManager();

  bool init();

  /// @brief 获取窗口物理尺寸（像素）
  Vec2i physicalExtent() const;

  /// @brief 获取窗口物理像素数量
  i32 physicalPixelCount() const;

  /// @brief 获取窗口宽高比
  f32 aspectRatio() const noexcept;

  /// @brief 窗口尺寸变化回调
  bool onResize();

  /// @brief 将最终颜色缓冲提交到 SDL
  void commitBuffer(const u32* packed_abgr8_buffer, i32 buffer_width, i32 buffer_height);

  /// @brief 获取 SDL 窗口句柄
  SDL_Window* SDLWindowHandler() const noexcept { return sdl_window_; }

private:
  bool createSDLWindow();
  bool createSDLRenderer();
  bool createSDLTexture();

  SDL_Window*   sdl_window_          = nullptr;
  SDL_Renderer* sdl_renderer_        = nullptr;
  SDL_Texture*  sdl_texture_present_ = nullptr;
};

} // namespaceMedium
