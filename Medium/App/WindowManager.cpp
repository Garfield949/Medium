///
/// @file   WindowManager.cpp
/// @author Garfield949
/// @date   2026/6/14 21:38
/// @brief  
///
#include "WindowManager.h"

#include <SDL3/SDL.h>

#include "Base/Log.h"
#include "Base/Color.h"
#include "App/WindowDefaults.h"

namespace Medium {

Vec2i WindowManager::physicalExtent() const {
  i32 w = 0, h = 0;
  SDL_GetWindowSizeInPixels(sdl_window_, &w, &h);
  return { w, h };
}

i32 WindowManager::physicalPixelCount() const {
  auto extent = physicalExtent();
  return extent.x() * extent.y();
}

f32 WindowManager::aspectRatio() const noexcept {
  auto extent = physicalExtent();
  return static_cast<f32>(extent.x()) / static_cast<f32>(extent.y());
}

WindowManager::~WindowManager() {
  // 这几个销毁函数如果传入空指针会直接返回，是安全的。
  SDL_DestroyTexture(sdl_texture_present_);
  SDL_DestroyRenderer(sdl_renderer_);
  SDL_DestroyWindow(sdl_window_);
  // 清理 SDL 已初始化的子系统和资源。
  SDL_Quit();
}

bool WindowManager::init() {
  // SDL_INIT_VIDEO 启用 SDL 与桌面显示环境相关的子系统，该标志位隐式地启动 SDL_INIT_EVENTS.
  if(!SDL_Init(SDL_INIT_VIDEO)) {
    Log::printLogPrefix("WindowManager", "init", true);
    fmt::print("{}\n", SDL_GetError());
    return false;
  }

  if(!createSDLWindow()) return false;
  if(!createSDLRenderer()) return false;
  if(!createSDLTexture()) return false;

  return true;
}

bool WindowManager::onResize() {
  return createSDLTexture();
}

void WindowManager::commitBuffer(const Color* packed_abgr8_buffer, i32 buffer_width, i32 buffer_height) {
  size_t bytes_cnt_row   = buffer_width  * sizeof(Color);
  size_t bytes_cnt_total = buffer_height * bytes_cnt_row;

  // 接收可写的像素内存地址。
  void* dst = nullptr;
  // 接收一行像素的总字节数。为了内存对齐 SDL 可能会在每行尾部进行 padding.
  // pitch 有间距、步距、相邻重复单元之间距离的意思。
  int row_pitch = 0;

  // 频繁更新帧缓冲，官方建议使用 SDL_LockTexture 锁定一个 SDL_TEXTUREACCESS_STREAMING 纹理的路径，而非 SDL_UpdateTexture.
  if(SDL_LockTexture(sdl_texture_present_, nullptr, &dst, &row_pitch)) {
    // 若没有 padding，则直接整块写入，可能会稍微快一些。
    if(static_cast<size_t>(row_pitch) == bytes_cnt_row)
      std::memcpy(dst, packed_abgr8_buffer, bytes_cnt_total);
    else // 否则一行一行写入。
      for(i32 y = 0; y < buffer_height; ++y)
        std::memcpy(
          // void 指针不支持算术运算，因为类型未知无法计算步长，需转型一下。
          static_cast<std::byte*>(dst) + y * row_pitch,
          packed_abgr8_buffer + y * buffer_width,
          bytes_cnt_row
        );
    
    SDL_UnlockTexture(sdl_texture_present_);
  }
  // 将缓冲绘制到 SDL_Renderer 上。
  SDL_RenderTexture(sdl_renderer_, sdl_texture_present_, nullptr, nullptr);
  // 将绘制好的离屏 buffer 提交到窗口，若开启垂直同步，该函数会阻塞一段时间才返回。
  SDL_RenderPresent(sdl_renderer_);
}

bool WindowManager::createSDLWindow() {
  sdl_window_ = SDL_CreateWindow(
    WindowDefaults::kDefaultWindowTitle,
    WindowDefaults::kDefaultWindowWidth,
    WindowDefaults::kDefaultWindowHeight,
    // SDL_WINDOW_HIGH_PIXEL_DENSITY: 尽可能让窗口适配高 DPI.
    SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
  );

  if(!sdl_window_) {
    Log::printLogPrefix("WindowManager", "createSDLWindow", true);
    fmt::print("{}\n", SDL_GetError());
    return false;
  }

  // 开启高 DPI 时，后端可能会对应渲染一个比给定屏幕逻辑尺寸大两倍的缓冲。比如屏幕尺寸为 1440x900，后端可能会提交
  // 一个 2880x1800 的屏幕缓冲：
  // 1. 假设设备屏幕本身就有 2880x1800 个物理像素，便可做到 2x2 的物理像素对应一个逻辑像素；
  // 2. 假设设备屏幕只有 1440x900 个物理像素，SDL 只会给一个相同尺寸的缓冲。
  if(SDL_GetWindowDisplayScale(sdl_window_) > 1) {
    Log::printLogPrefix("WindowManager", "createSDLWindow", true);
    fmt::print("Scale enabled\n");
  }

  return true;
}

bool WindowManager::createSDLRenderer() {
  // 第二个参数是指定一个渲染后端的名称，比如 "metal". 不用管，它会自动选择。
  sdl_renderer_ = SDL_CreateRenderer(sdl_window_, nullptr);

  if(!sdl_renderer_) {
    Log::printLogPrefix("WindowManager", "createSDLRenderer", true);
    fmt::print("{}\n", SDL_GetError());
    return false;
  }

  // 垂直同步。
  SDL_SetRenderVSync(sdl_renderer_, 1);

  return true;
}

bool WindowManager::createSDLTexture() {
  auto extent = physicalExtent();

  SDL_Texture* new_texture = SDL_CreateTexture(
    sdl_renderer_,
    SDL_PIXELFORMAT_ABGR8888,
    SDL_TEXTUREACCESS_STREAMING, // 材质作为颜色缓冲区会频繁变换，使用 STREAMING 而非 STATIC.
    extent.x(),
    extent.y()
  );

  if(!new_texture) {
    Log::printLogPrefix("WindowManager", "createSDLTexture", true);
    fmt::print("{}\n", SDL_GetError());
    return false;
  }

  // 销毁放在创建之后，否则若创建失败返回并 quit 会又调用一次 SDL_DestroyTexture 可能会 double free.
  SDL_DestroyTexture(sdl_texture_present_);

  sdl_texture_present_ = new_texture;

  return true;
}

} // namespace Medium