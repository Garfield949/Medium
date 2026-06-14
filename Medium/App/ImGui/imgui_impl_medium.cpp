///
/// @file   imgui_impl_medium.cpp
/// @author Garfield949
/// @date   2026/6/5 17:44
/// @brief  
///
#include "DearImGui/imgui.h"

#ifndef IMGUI_DISABLE

#include "imgui_impl_medium.h"

bool ImGui_ImplMedium_Init() {
  
}

void ImGui_ImplMedium_Shutdown() {

}

void ImGui_ImplMedium_NewFrame() {

}

void ImGui_ImplMedium_RenderDrawData(ImDrawData* draw_data) {
  // 窗口最小化时直接 early out.
  if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
    return;

  
}

#endif // #ifndef IMGUI_DISABLE
