///
/// @file   ImGuiInspector.cpp
/// @author Garfield949
/// @date   2026/6/21 19:48
/// @brief
///
#include "ImGuiInspector.h"
#include "imgui_impl_medium.h"

#include <cstddef>
#include <imgui.h>
#include <SDL3/SDL.h>

#include "App/FPSController.h"
#include "App/WindowManager.h"
#include "Scene/Camera.h"
#include "Scene/SceneManager.h"
#include "Scene/Light/DirectionalLight.h"
#include "Pipeline/RenderStatus.h"
#include "Pipeline/RenderContext.h"
#include "Pipeline/RenderManager.h"
#include "Pipeline/Shadow/ShadowManager.h"

namespace Medium {

void ImGuiInspector::run() {
  showStatisticsWindow();
  showInspectorWindow();

  // 当 Debug / Inspector 面板折叠时，若取消了 Cast Shadows，无法通过 showInspectorWindow 中的代码
  // 同步 ShadowManager 的相关状态，所以需要在外部特判一下。
  if(!render_manager->shadowingEnabled()) {
    auto& shadow_manager = render_manager->shadowManager();
    
    shadow_manager.requestAtlasDebugView(false);
    shadow_manager.enableCascadeBandView(false);
    shadow_manager.freezeCSM(false);

    show_csm_atlas_ = false;
  }
  
  if(show_csm_atlas_) showCSMAtlasWindow();
}

void ImGuiInspector::showStatisticsWindow() {
  ImGui::SetNextWindowPos(ImVec2{window_margin_, window_margin_});  // 默认 ImGuiCond = ImGuiCond_Always.
  ImGui::SetNextWindowSize(ImVec2{w_statistics_window_, 0.0f});     // 高度设为 0，让 ImGui 自动计算。

  static bool collapsed = false;
  
  // ImGuiMod_Ctrl 在 macOS 下映射为 command. 
  // 后面的 flag 保证无论 statistics 窗口是否为焦点，都会响应快捷键。
  if(ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_1, ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_RouteOverFocused)) {
    collapsed = !collapsed;
    ImGui::SetNextWindowCollapsed(collapsed);  // 按下快捷键才设置窗口是否折叠，否则放到外面会覆盖鼠标的点击。
  }
  
  if(!ImGui::Begin("Statistics - Ctrl/Cmd+1", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) { 
    ImGui::End();  // 如果折叠了窗口则进行 early out.
    return;
  }
  
  ImGuiIO& io = ImGui::GetIO(); 
  
  ShadowManager& shadow_manager = render_manager->shadowManager();
  const RenderStatus& render_status = render_manager->renderStatus();

  Vec2i display_size    = window_manager->physicalExtent();
  i32   physical_pixels = window_manager->physicalPixelCount();
  f32   pixel_density   = SDL_GetWindowDisplayScale(window_manager->SDLWindowHandler());

  ImGui::SeparatorText("Frame Time");
  ImGui::Text("%.2f ms / %.1f fps", 1000.0f / io.Framerate, io.Framerate);
  ImGui::SeparatorText("Resolution");
  ImGui::Text("Window   : [%.0f x %.0f]", io.DisplaySize.x, io.DisplaySize.y);
  ImGui::Text("Buffer   : [%d x %d] (%.0fx scale / hidpi %s)",
    display_size.x(), display_size.y(), pixel_density,
    SDL_GetWindowDisplayScale(window_manager->SDLWindowHandler()) > 1 ? "enabled" : "disabled");
  ImGui::Text("CSM Atlas: [%d x %d]", shadow_manager.cascadeTileRes() * 2, shadow_manager.cascadeTileRes() * 2);

  ImGui::SeparatorText("Draw Calls");
  ImGui::Text("Total                  : %d", render_status.statistics.totalDrawCalls());
  ImGui::Text("Shadow Pass Opaque     : %d", render_status.statistics.shadow_opaque_draw_calls);
  ImGui::Text("Main Pass   Opaque     : %d", render_status.statistics.scene_opaque_draw_calls);
  ImGui::Text("Main Pass   Transparent: %d", render_status.statistics.scene_transp_draw_calls);

  ImGui::SeparatorText("Main Pass Workload");
  ImGui::Text("Visible Polygons: %d", render_status.statistics.visible_polygons);
  if(physical_pixels * 1.1 <= render_status.statistics.written_pixels) {
    ImGui::Text("Colored Pixels  :");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "%d", render_status.statistics.written_pixels);
    ImGui::SameLine();
    ImGui::Text(">> Physical Pixels (%d)", physical_pixels);
  }
  else {
    ImGui::Text("Colored Pixels  : %d", render_status.statistics.written_pixels);
  }

  h_statistics_window_ = ImGui::GetWindowHeight();

  ImGui::End();
}

void ImGuiInspector::showInspectorWindow() {
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  
  ImGui::SetNextWindowPos(ImVec2{io.DisplaySize.x - w_inspector_window_ - window_margin_, window_margin_});
  ImGui::SetNextWindowSize(ImVec2{w_inspector_window_, 0.0f});
  ImGui::SetNextWindowSizeConstraints(
      ImVec2(w_inspector_window_, 0),                    // 最小
      ImVec2(w_inspector_window_, io.DisplaySize.y - 30) // 最大
  );

  static bool collapsed = false;
  
  if(ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_2, ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_RouteOverFocused)) {
    collapsed = !collapsed;
    ImGui::SetNextWindowCollapsed(collapsed);
  }

  if(!ImGui::Begin("Inspector - Ctrl/Cmd+2", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
    ImGui::End();
    return;
  }
  showScenePanel();
  showDebugPanel();
  showFPSControllerInfoPanel();
  showAboutPanel();
  ImGui::End();
}

void ImGuiInspector::showCSMAtlasWindow() {
  ImGui::SetNextWindowPos({15, h_statistics_window_ + 30}, ImGuiCond_Always);
  ImGui::SetNextWindowSize({315, 335}, ImGuiCond_Always);
  if(ImGui::Begin("CSM Atlas Debug View")) {
    ImGui::Image(ImGui_ImplMedium_Texture2DPtrToImTextureID(render_manager->shadowManager().CSMAtlasDebugImage()),
      ImVec2{300.0f, 300.0f},
      ImVec2{0.0f, 1.0f},
      ImVec2{1.0f, 0.0f}
    );
  }
  ImGui::End();
}

void ImGuiInspector::showScenePanel() {
  if(ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Spacing();
    ImGui::Spacing();
    
    static const char* scene_filenames[]     {"sponza.lua", "minecraft.lua", "porsche.lua", "snake.lua", "house.lua", "teapot.lua"};
    static const char* scene_display_names[] {"Sponza", "Minecraft", "Porsche", "Snake Statue", "House", "Teapot"};
    static int selected_idx = 0;
    static bool scene_switched = true;
  
    ImGui::PushItemWidth(150.0f);
    if(ImGui::BeginCombo(" Scene List", scene_display_names[selected_idx])) {
      for(int n = 0; n < IM_COUNTOF(scene_display_names); n++) {
        // 这句用于配合下面的 SetItemDefaultFocus.
        const bool is_selected = (selected_idx == n);
        // 显示一个可选项，文字为 "scene_names[n]"。若用户点击该项，则设置 selected_idx，未点击则跳出 if.
        if(ImGui::Selectable(scene_display_names[n], is_selected) && selected_idx != n) {
          if(scene_manager->switchScene(scene_filenames[n], window_manager->aspectRatio())) {
            render_manager->bindNewScene(scene_manager->activeScene());
            controller->bindNewHost(scene_manager->activeScene()->mainCamera());
            selected_idx = n;
            scene_switched = true;

            show_csm_atlas_ = false;

            auto& shadow_manager = render_manager->shadowManager();
            shadow_manager.requestAtlasDebugView(false);
            shadow_manager.enableCascadeBandView(false);
            shadow_manager.freezeCSM(false);
          }
        }
        // 若当前项已是选中项，设置为焦点。
        if(is_selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    
    ImGui::SeparatorText("Directional Light");
    DirectionalLight* dir_light = scene_manager->activeScene()->directionalLight();

    // 真值源用一个未归一化的静态变量。否则如果去读方向光的 dir 作为真值的话，每次在 ui 面板设置
    // 一个值后，归一化会导致读回来的三个分量出现变化，交互体验不太好。
    static Vec3f light_dir;
    if(scene_switched) {
      light_dir = dir_light->dir();
      scene_switched = false;
    }
    
    if(ImGui::SliderFloat(" Dir X", &light_dir.x(), -1.0f, 1.0f)) dir_light->setDir(light_dir);
    if(ImGui::SliderFloat(" Dir Y", &light_dir.y(), -1.0f, 0.0f)) dir_light->setDir(light_dir);
    if(ImGui::SliderFloat(" Dir Z", &light_dir.z(), -1.0f, 1.0f)) dir_light->setDir(light_dir);
    ImGui::ColorEdit3(" Color", dir_light->color.data());

    ImGui::Spacing();

    ImGui::Checkbox("Cast Shadows", &dir_light->cast_shadows);
  
    ImGui::SeparatorText("Main Camera");

    auto camera = scene_manager->activeScene()->mainCamera();
    f32 fov = camera->fov();
    if(ImGui::SliderFloat(" Fov", &fov,  50.0f, 120.0f))
      camera->setFOV(fov);
    
    ImGui::Spacing();
  }
}

void ImGuiInspector::showDebugPanel() {
  if(ImGui::CollapsingHeader("Debug")) {
    // ---------- Draw Mode ----------
    ImGui::SeparatorText("Draw Mode");

    DrawMode mode = render_manager->drawMode();

    if(ImGui::RadioButton("Shaded", mode == DrawMode::Shaded))
      render_manager->setDrawMode(DrawMode::Shaded);

    ImGui::SameLine();

    if(ImGui::RadioButton("Wireframe", mode == DrawMode::Wireframe))
      render_manager->setDrawMode(DrawMode::Wireframe);

    // ---------- Overlays ----------
    ImGui::SeparatorText("Overlays");
    bool show_aabb = render_manager->drawAABBs();
    if(ImGui::Checkbox("AABBs", &show_aabb))
      render_manager->setDrawAABBs(show_aabb);

    // ---------- Cascade Shadow Mapping 禁用态起始 ----------
    ImGui::BeginDisabled(!render_manager->shadowingEnabled());

    ImGui::SeparatorText("Cascaded Shadow Mapping");
    
    ShadowManager& shadow_manager = render_manager->shadowManager();

    // ---------- Cascade Resolution 禁用态起始 ----------
    ImGui::BeginDisabled(shadow_manager.cascadeBandViewEnabled() || shadow_manager.CSMFrozen());
    
    static const i32   res[] = {256, 512, 1024, 2048};
    static const char* res_display_names[] = {"256", "512", "1024", "2048"};
    static int selected_idx = 2;

    ImGui::PushItemWidth(150.0f);
    if(ImGui::BeginCombo(" Cascade Resolution", res_display_names[selected_idx])) {
      for(int n = 0; n < IM_COUNTOF(res_display_names); n++) {
        const bool is_selected = (selected_idx == n);
        if(ImGui::Selectable(res_display_names[n], is_selected) && selected_idx != n) {
          shadow_manager.setCascadeTileRes(res[n]);
          selected_idx = n;
        }
        if(is_selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::EndDisabled();
    // ---------- Cascade Resolution 禁用态结束 ----------

    ImGui::Spacing();

    if(ImGui::Checkbox("Show CSM Atlas", &show_csm_atlas_))
      shadow_manager.requestAtlasDebugView(show_csm_atlas_);

    bool enable_cascade_band_view = shadow_manager.cascadeBandViewEnabled();
    if(ImGui::Checkbox("Enable Cascade Band View", &enable_cascade_band_view))
      shadow_manager.enableCascadeBandView(enable_cascade_band_view);

    bool frozen = shadow_manager.CSMFrozen();
    if(ImGui::Checkbox("Freeze", &frozen))
      shadow_manager.freezeCSM(frozen);

    ImGui::Separator();

    ImGui::BeginDisabled(frozen);
    
    i32 cascade_count   = shadow_manager.cascadeCount();
    f32 max_shadow_dist = shadow_manager.maxShadowDistance();

    ImGui::PushItemWidth(200.0f);
    
    if(ImGui::SliderFloat(" Max Shadow Distance", &max_shadow_dist,  0.2f, 50.0f))
      shadow_manager.setMaxShadowDistance(max_shadow_dist);

    if(ImGui::SliderInt(" Cascade Count", &cascade_count, 1, 4))
      shadow_manager.setCascadeCount(cascade_count);
    
    ImGui::PopItemWidth();

    ImGui::EndDisabled();

    for(i32 i = 0; i < cascade_count; ++i)
      ImGui::Text("Cascade %d: [%.1f ~ %.1f]\n", i, shadow_manager.cascadeSlices()[i],
                                                    shadow_manager.cascadeSlices()[i+1]);
    ImGui::Spacing();

    ImGui::EndDisabled();
    // ---------- Cascade Shadow Mapping 禁用态结束 ----------
  }
}

void ImGuiInspector::showFPSControllerInfoPanel() {
  if(ImGui::CollapsingHeader("FPS Controller")) {
    ImGui::SeparatorText("Instructions");
    ImGui::Text("Move       : Keyboard WASD");
    ImGui::Text("Up/Down    : Space / Shift");
    ImGui::Text("Look around: Drag RMB");
    ImGui::Separator();
    ImGui::SliderFloat(" Move  Speed", &controller->moveSpeed(),  0.0f, 10.0f);
    ImGui::SliderFloat(" Mouse Speed", &controller->mouseSpeed(), 0.0f, 1.0f);
    ImGui::Spacing();
  }
}

void ImGuiInspector::showAboutPanel() {
  if(ImGui::CollapsingHeader("About")) {
    ImGui::SeparatorText("ImGui Backends");
    ImGui::Text("Platform: SDL3");
    ImGui::Text("Renderer: Medium CPU Pipeline");
    ImGui::SeparatorText("Third-Party");
    ImGui::Text("Dear ImGui    - MIT");
    ImGui::Text("Eigen         - MPL2.0");
    ImGui::Text("fmt           - MIT");
    ImGui::Text("Lua           - Lua");
    ImGui::Text("LuaBridge3    - MIT");
    ImGui::Text("SDL3          - zlib");
    ImGui::Text("stb_image     - MIT");
    ImGui::Text("TinyObjLoader - MIT");
  }
}

} // namespace Medium