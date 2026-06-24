///
/// @file   imgui_impl_medium.h
/// @author Garfield949
/// @date   2026/6/5 17:15
/// @brief  
///
#pragma once

#include <imgui.h> // IMGUI_IMPL_API

// 只要定义了 IMGUI_DISABLE 宏，整个 ImGui 库会被完全禁用。猜测某些项目集成 ImGui 只是为了调试，等
// shipping 的时候就会整个关闭。
#ifndef IMGUI_DISABLE

namespace Medium {
  class FrameBuffer;
  class Texture2D;
}

IMGUI_IMPL_API bool ImGui_ImplMedium_Init();
IMGUI_IMPL_API void ImGui_ImplMedium_Shutdown();
IMGUI_IMPL_API void ImGui_ImplMedium_NewFrame();
IMGUI_IMPL_API void ImGui_ImplMedium_RenderDrawData(ImDrawData* draw_data,
                                                    Medium::FrameBuffer& framebuffer);

#endif // #ifndef IMGUI_DISABLE