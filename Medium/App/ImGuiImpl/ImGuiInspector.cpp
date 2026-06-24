///
/// @file   ImGuiInspector.cpp
/// @author Garfield949
/// @date   2026/6/21 19:48
/// @brief  
///
#include "ImGuiInspector.h"

#include <imgui.h>
#include <SDL3/SDL.h>

#include "App/FPSController.h"
#include "App/WindowManager.h"
#include "Pipeline/RenderStat.h"
#include "Scene/Camera.h"
#include "Scene/SceneManager.h"
#include "Pipeline/RenderManager.h"

namespace Medium {

void ImGuiInspector::show() {
  ImGuiIO& io = ImGui::GetIO(); (void)io;

  // 默认 ImGuiCond = ImGuiCond_Always.
  ImGui::SetNextWindowPos(ImVec2{io.DisplaySize.x - w_inspector - 15, 15.0f});
  ImGui::SetNextWindowSize(ImVec2{w_inspector, io.DisplaySize.y - io.DisplaySize.y * 0.2f});

  // Inspector start.
  if(!ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
    // 优化：如果用户折叠了 Inspector, 则进行 early out.
    ImGui::End();
    return;
  }

  showBasicInfoPanel();
  showDebugPanel();
  showSceneInfoPanel();
  showCameraInfoPanel();
  showAboutPanel();

  // Inspector end.
  ImGui::End();
}

void ImGuiInspector::showBasicInfoPanel() {
  if(ImGui::CollapsingHeader("Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    const RenderStat& render_status = render_manager->renderStat();
  
    Vec2i display_size    = window_manager->physicalExtent();
    u32   physical_pixels = window_manager->physicalPixelCount();
    ImGui::Text("Frame Time : %.2f ms / %.1f fps", 1000.0f / io.Framerate, io.Framerate);
    ImGui::SeparatorText("Resolution");
    ImGui::Text("Window Size: %.0f x %.0f", io.DisplaySize.x, io.DisplaySize.y);
    ImGui::Text("Back Buffer: %d x %d (hidpi %s)",
      display_size.x(), display_size.y(),
      SDL_GetWindowPixelDensity(window_manager->SDLWindowHandler()) > 1 ? "enabled" : "disabled");
    ImGui::SeparatorText("Draw Calls");
    ImGui::Text("Total                  : %u", render_status.statistics.totalDrawCalls());
    ImGui::Text("Shadow Pass Opaque     : %u", render_status.statistics.shadow_opaque_draw_calls);
    ImGui::Text("Main Pass   Opaque     : %u", render_status.statistics.scene_opaque_draw_calls);
    ImGui::Text("Main Pass   Transparent: %u", render_status.statistics.scene_transp_draw_calls);
    ImGui::SeparatorText("Main Pass Workload");
    ImGui::Text("Visible Triangles: %u", render_status.statistics.visible_triangles);
    if(physical_pixels * 1.1 <= render_status.statistics.written_pixels)
      ImGui::Text("Written Pixels   : %u >> Physical Pixels (%u)", render_status.statistics.written_pixels, physical_pixels);
    else
      ImGui::Text("Written Pixels   : %u", render_status.statistics.written_pixels);
  } 
}

void ImGuiInspector::showDebugPanel() {
  if(ImGui::CollapsingHeader("Debug", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Cascaded Shadow Mapping")) {
      const RenderStat& render_status = render_manager->renderStat();
      
      i32 cascade_count   = render_status.csm.cascade_count;
      f32 max_shadow_dist = render_status.csm.max_shadow_distance;
      
      ImGui::PushItemWidth(200.0f);
      ImGui::SliderFloat("Max Shadow Distance", &max_shadow_dist,  0.2f, 50.0f);
      ImGui::SliderInt("Cascade Count", &cascade_count, 1, 4);
      ImGui::PopItemWidth();

      render_manager->shadowManager().updateCSMSlices(max_shadow_dist, cascade_count);

      for(u32 i = 0; i < render_status.csm.cascade_count; ++i)
        ImGui::Text("Cascade %u: [%.1f ~ %.1f]\n", i, render_status.csm.csm_slices[i], 
                                                      render_status.csm.csm_slices[i+1]);
      ImGui::TreePop();
    }
  }
}

void ImGuiInspector::showSceneInfoPanel() {
  if(ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
    static int selected_idx = 0;
    
    const char* scene_file_names[] = 
      {"sponza", "porsche", "bmw", "minecraft", "F1", "snake", "house", "teapot"};

    const char* scenes[] = 
      {"Crytek Sponza", "Porsche", "Bmw", "Minecraft", "F1", "Snake Statue", "House", "Teapot"};

    const char* preview_value = scenes[selected_idx];
  
    if(ImGui::BeginCombo("Scene list", preview_value)) {
      for(int n = 0; n < IM_COUNTOF(scenes); n++) {
        // 这句用于配合下面的 SetItemDefaultFocus.
        const bool is_selected = (selected_idx == n);
        // 显示一个可选项，文字为 "scenes[n]"。若用户点击该项，则设置 selected_idx，未点击则跳出 if.
        if(ImGui::Selectable(scenes[n], is_selected) && selected_idx != n) {
          selected_idx = n;
          if(scene_manager->switchScene(scene_file_names[selected_idx], window_manager->aspectRatio())) {
            render_manager->bindNewScene(scene_manager->activeScene());
            controller->bindNewHost(scene_manager->activeScene()->camera());
          }
        }
        // 若当前项已是选中项，设置为焦点。
        if(is_selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
  }
}

void ImGuiInspector::showFPSControllerInfoPanel() {
  if(ImGui::CollapsingHeader("FPS Controller")) {
    ImGui::SeparatorText("Instructions");
    ImGui::Text("Move       : Keyboard WASD");
    ImGui::Text("Up/Down    : Space / Shift");
    ImGui::Text("Look around: Drag RMB");
    ImGui::Separator();
    ImGui::SliderFloat("Move  Speed", &controller->moveSpeed(),  0.0f, 10.0f);
    ImGui::SliderFloat("Mouse Speed", &controller->mouseSpeed(), 0.0f, 1.0f);
  }
}

void ImGuiInspector::showCameraInfoPanel() {
  if(ImGui::CollapsingHeader("Camera")) {
    ImGui::SliderFloat("Fov", &scene_manager->activeScene()->camera()->fov(),  50.0f, 120.0f);
  }
}

void ImGuiInspector::showAboutPanel() {
  if(ImGui::CollapsingHeader("About")) {
    ImGui::Text("ImGui Platform Backend: SDL3");
    ImGui::Text("ImGui Renderer Backend: Medium CPU Pipeline");
  }
}

} // namespace Medium