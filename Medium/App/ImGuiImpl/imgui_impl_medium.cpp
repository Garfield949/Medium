///
/// @file   imgui_impl_medium.cpp
/// @author Garfield949
/// @date   2026/6/5 17:44
/// @brief  
///
#ifndef IMGUI_DISABLE
#include "imgui_impl_medium.h"

#include <cstddef>
#include <cstring>

#include "Base/Color.h"
#include "Base/Geometry/Rect.h"
#include "Resources/Texture2D.h"
#include "Pipeline/DrawCore.h"
#include "Pipeline/FrameBuffer.h"
#include "Pipeline/Shader/ShaderVariant.h"

namespace {
  
struct ImGui_ImplMedium_Data {
  Medium::ShaderVariant shader = Medium::ImGuiShader{};
  std::vector<Medium::Vertex> vertices;
  std::vector<size_t> indices;
};

ImGui_ImplMedium_Data* ImGui_ImplMedium_GetBackendData() {
  ImGuiContext* ctx = ImGui::GetCurrentContext();
  if(!ctx) return nullptr;
  return static_cast<ImGui_ImplMedium_Data*>(ImGui::GetIO().BackendRendererUserData);
}

// 如果 src_tex 需要更新某个区域，则同步写入其绑定的 dst_tex 对应区域。
void ImGui_ImplMedium_WriteTexture(Medium::Texture2D* dst_tex, 
                                    ImTextureData* src_tex,
                                    const ImTextureRect& rect) {
  // ImTextureData->Format (ImTextureFormat) 分为 ImTextureFormat_RGBA32 (字面意义的内存布局：
  // 低地址 -> RR GG BB AA -> 高地址，即 Packed ABGR8) 和 ImTextureFormat_Alpha8. 后者是为计算
  // 能力较弱的嵌入式设备准备的，暂时无视掉这个分支。
  if(src_tex->Format == ImTextureFormat_RGBA32) {
    size_t w_tex = src_tex->Width;
    size_t bytes_per_row_rect = rect.w * src_tex->BytesPerPixel;

    auto dst_data = dst_tex->packedAbgr8Data();
    auto src_data = src_tex->Pixels;
    
    for(unsigned short i = 0; i < rect.h; ++i) {
      size_t offset = w_tex * (rect.y + i) + rect.x;
      std::memcpy(
        dst_data + offset,
        src_data + offset * 4,
        bytes_per_row_rect
      );
    }
  }
}

void ImGui_ImplMedium_ProcessImTextureData(ImTextureData* tex_data) {
  // ImGui 创建了一个新材质，并要求绑定到自定义 Texture2D 上。
  if(tex_data->Status == ImTextureStatus_WantCreate) {
    auto medium_tex = new Medium::Texture2D{Medium::PixelFormat::ABGR8, Medium::ColorSpace::SRGB};
    if(!medium_tex->resize(tex_data->Width, tex_data->Height)) {
      delete medium_tex;
      return;
    }
    ImGui_ImplMedium_WriteTexture(
      medium_tex, tex_data,
      {0, 0, static_cast<unsigned short>(tex_data->Width), static_cast<unsigned short>(tex_data->Height)}
    );
    tex_data->BackendUserData = medium_tex;
    tex_data->SetTexID(ImGui_ImplMedium_Texture2DPtrToImTextureID(medium_tex));
    tex_data->SetStatus(ImTextureStatus_OK);
  }
  
  else if(tex_data->Status == ImTextureStatus_WantUpdates) {
    auto medium_tex = static_cast<Medium::Texture2D*>(tex_data->BackendUserData);
    for(const ImTextureRect& rect: tex_data->Updates)
      ImGui_ImplMedium_WriteTexture(medium_tex, tex_data, rect);
    tex_data->SetStatus(ImTextureStatus_OK);
  }
  
  else if(tex_data->Status == ImTextureStatus_WantDestroy) {
    auto medium_tex = static_cast<Medium::Texture2D*>(tex_data->BackendUserData);
    delete medium_tex;
    tex_data->BackendUserData = nullptr;
    tex_data->SetTexID(ImTextureID_Invalid);
    tex_data->SetStatus(ImTextureStatus_Destroyed);
  }
}

Medium::LinearColor ImGui_ImplMedium_PackedToLinearColor(ImU32 packed) {
  Medium::Color c;
  c.data = packed;
  return c.toLinear(Medium::ColorSpace::SRGB);
}

Medium::Rect ImGui_ImplMedium_ClipRectToRect(const ImDrawCmd*  cmd,
                                             const ImDrawData* draw_data,
                                             int w_fb_physical,
                                             int h_fb_physical) // fb = framebuffer.
{
  const ImVec2 display_pos  = draw_data->DisplayPos;
  const ImVec2 buffer_scale = draw_data->FramebufferScale; // HiDPI 缩放倍数。
  // ImDrawCmd.ClipRect，类型为 ImVec4(x1,y1,x2,y2)，代表左上角(x1,y1)，右下角(x2,y2).
  // ClipRect 表示当前 ImDrawCmd 所描述三角形集合（可能是 UI 面板的某部分）的矩形范围，若超出定
  // 义的视口则被钳制。
  float x_min = (cmd->ClipRect.x - display_pos.x) * buffer_scale.x;
  float y_min = (cmd->ClipRect.y - display_pos.y) * buffer_scale.y;
  float x_max = (cmd->ClipRect.z - display_pos.x) * buffer_scale.x;
  float y_max = (cmd->ClipRect.w - display_pos.y) * buffer_scale.y;

  int x0 = std::max(static_cast<int>(std::floor(x_min)), 0);
  int y0 = std::max(static_cast<int>(std::floor(y_min)), 0);
  int x1 = std::min(static_cast<int>(std::ceil(x_max)), w_fb_physical);
  int y1 = std::min(static_cast<int>(std::ceil(y_max)), h_fb_physical);
  // ClipRect 描述的是原点在左上角的屏幕空间区域，转换到 Rect 时 y 要翻转一下。
  return { x0, h_fb_physical - y1, x1 - x0, y1 - y0 };
}
  
} // 匿名命名空间，控制仅单文件可见


ImTextureID ImGui_ImplMedium_Texture2DPtrToImTextureID(const Medium::Texture2D* ptr) {
  // ImTextureID 本身是个无符号整数，所以加一层转 uintptr_t (大小足以保存一个指针的无符号数) 完善「地址转整数」
  // 这个语义，去掉也没问题。
  return static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(ptr));
}

const Medium::Texture2D* ImGui_ImplMedium_ImTextureIDtoTexture2DPtr(ImTextureID id) {
  return reinterpret_cast<Medium::Texture2D*>(static_cast<uintptr_t>(id));
}

// 由用户调用
void ImGui_ImplMedium_Init() {
  // ImGuiIO 保存后端、输入状态、显式尺寸、功能标志等信息。
  ImGuiIO& io = ImGui::GetIO();
  // 检查渲染后端是否已注册过（防止二次调用Init?）。
  IM_ASSERT(io.BackendRendererUserData == nullptr);
  // 用 ImGui 的内存分配器创建一个 Medium 自定义的后端数据实例。
  ImGui_ImplMedium_Data* bd = IM_NEW(ImGui_ImplMedium_Data)();
  // 挂载到 io 上。
  io.BackendRendererUserData = bd;
  // 设置渲染后端，也就是 Medium 的名称，用于调试、Log 等。
  io.BackendRendererName = "imgui_impl_medium";
  // 启用顶点偏移支持，Medium 的管线已支持。
  // ImGui 的默认索引类型是 16 位 typedef unsigned short，最大值只支持 65535，若总顶点数超过它就没法
  // 再索引了。VtxOffset 作用是给定一个 vertices 的基偏移量，再加一个 IdxOffset:
  // vertices[VtxOffset + indices[IdxOffset + i]].
  // 具体看 DrawCore.h 的注释。
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
  // 1.92 版本引入的标记。要求渲染后端处理 ImDrawData->Texture 的纹理更新请求；关闭时还需遍历
  // ImGui::GetPlatformIO().Textures 销毁纹理。
  io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
}

// 用户手动调用，销毁资源和句柄。
void ImGui_ImplMedium_Shutdown() {
  ImGui_ImplMedium_Data* bd = ImGui_ImplMedium_GetBackendData();
  IM_ASSERT(bd != nullptr && "No render backend to shutdown, or already shutdown?");
  
  for(ImTextureData* tex_data: ImGui::GetPlatformIO().Textures) {
    if(tex_data->BackendUserData) {
      delete static_cast<Medium::Texture2D*>(tex_data->BackendUserData);
      tex_data->BackendUserData = nullptr;
      tex_data->SetTexID(ImTextureID_Invalid);
      tex_data->SetStatus(ImTextureStatus_Destroyed);
    }
  }
    
  ImGuiIO& io = ImGui::GetIO();
  io.BackendRendererName = nullptr;
  io.BackendRendererUserData = nullptr;
  io.BackendFlags &= ~ImGuiBackendFlags_RendererHasVtxOffset;
  io.BackendFlags &= ~ImGuiBackendFlags_RendererHasTextures;

  // 没什么实际效果。
  ImGui::GetPlatformIO().ClearRendererHandlers();

  IM_DELETE(bd);
}

// 用户手动调用，一般用于每帧开始前的资源/状态准备，貌似啥也不用做。
void ImGui_ImplMedium_NewFrame() {
  ImGui_ImplMedium_Data* bd = ImGui_ImplMedium_GetBackendData();
  // 防编译器警告。
  (void)bd;
  // 从 SDL3 实现那里复制来的。
  IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplSDL3_Init()?");
}

// 用户手动调用，将 ImGui 资源转换成 Medium 自定义格式，并送入管线渲染。
void ImGui_ImplMedium_RenderDrawData(ImDrawData* draw_data,
                                     Medium::FrameBuffer& framebuffer) {
  // 窗口最小化时直接 early out.
  if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
    return;

  // 获取在 Init 时绑定的 Medium 自定义后端数据。
  ImGui_ImplMedium_Data* bd = ImGui_ImplMedium_GetBackendData();
  IM_ASSERT(bd != nullptr && draw_data != nullptr);

  // ImDrawData 可能每帧都会变（窗口位置、鼠标悬停、裁剪区域、文字光标、动画、输入状态等），所以
  // 重置接收数据的容器。
  bd->vertices.clear();
  bd->indices.clear();
  bd->vertices.reserve(static_cast<size_t>(draw_data->TotalVtxCount));
  bd->indices.reserve(static_cast<size_t>(draw_data->TotalIdxCount));

  // 查询 ImGui 是否有材质更新请求。
  if(draw_data->Textures)
    for(ImTextureData* tex_data: *draw_data->Textures) 
      if(tex_data->Status != ImTextureStatus_OK)
        ImGui_ImplMedium_ProcessImTextureData(tex_data);

  const Medium::RectF viewport{
    .x = 0.0f,
    .y = 0.0f,
    .w = framebuffer.widthF(),
    .h = framebuffer.heightF()
  };

  Medium::RenderState render_state{
    .enable_depth_test  = false,
    .enable_depth_write = false,
    .enable_color_write = true,
    .draw_mode          = Medium::DrawMode::Shaded,
    .cull_mode          = Medium::CullMode::Off,
    .blend_mode         = Medium::BlendMode::PremultipliedOver,
    .color_write_target = Medium::ColorWriteTarget::Present // UI 叠加在已经 tonemap 的 present 缓冲上。
  };

  // ShaderVariant 特化。
  auto& shader = std::get<Medium::ImGuiShader>(bd->shader);

  const int w_fb_physical = static_cast<int>(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
  const int h_fb_physical = static_cast<int>(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);

  // - 一份 ImDrawData 有多个 ImDrawList，一个 ImDrawList 有多个 ImDrawCmd;
  // - ImDrawList：提供一组顶点缓冲 VtxBuffer 和索引缓冲 IdxBuffer;
  // - ImDrawCmd： 描述如何绘制其中一段索引，包括 IdxOffset、VtxOffset、ClipRect、TextureId;
  // - 可将 ImDrawList 理解为 Medium::Mesh, ImDrawCmd 理解为 Medium::SubMesh.
  for(int i = 0; i < draw_data->CmdLists.Size; ++i) {
    const ImDrawList* cmd_list = draw_data->CmdLists[i];

    // 设置当前 DrawList 的顶点和索引在大数组中的基偏移：
    // - 初始 bd->vertices 和 bd->indices 为空，则偏移均为 0;
    // - 每个 DrawList 包含一批顶点和索引，占据大数组中的一段，所以当前 DrawList 的基偏移应为上一 DrawList 所占据段的末尾，即大数组的 size;
    // - 始终记住，传入 Medium 管线（见 DrawCore.h 的 IndexedDrawView）是一整个大数组，需手动进行偏移计算。
    const size_t vertex_base = bd->vertices.size();
    const size_t index_base  = bd->indices.size();

    // ImDrawList 层级取顶点。
    for(const ImDrawVert& src: cmd_list->VtxBuffer) {
      Medium::Vertex vert;
      vert.pos   = {src.pos.x, src.pos.y, 0.0f};
      vert.uv    = {src.uv.x , src.uv.y};
      vert.color = ImGui_ImplMedium_PackedToLinearColor(src.col);
      bd->vertices.push_back(vert);
    }
    // ImDrawList 层级取索引。
    for(ImDrawIdx idx: cmd_list->IdxBuffer)
      bd->indices.push_back(static_cast<Medium::u32>(idx));
 
    for(const ImDrawCmd& cmd: cmd_list->CmdBuffer) {
      // TODO: 这段暂时不知道是干嘛的。
      if(cmd.UserCallback) {
        if(cmd.UserCallback != ImGui::GetPlatformIO().DrawCallback_ResetRenderState)
          cmd.UserCallback(cmd_list, &cmd);
        continue;
      }

      Medium::Rect scissor = ImGui_ImplMedium_ClipRectToRect(&cmd, draw_data, w_fb_physical, h_fb_physical);
      if(!scissor.valid()) continue;

      // 取出当前 cmd 的纹理。
      // 如果没有个性化需求（比如插入图片等），纹理永远是字体 atlas，原因：UI 的 矩形、边框、线条都是用白色 texel 乘顶点颜色画出来的，
      // 而这个所谓的 「白色 texel」其实是 ImGui 生成的字体 atlas 纹理的一小块纯白区域。几乎所有控件的绘制都依赖 font atlas.
      const auto* texture = ImGui_ImplMedium_ImTextureIDtoTexture2DPtr(cmd.GetTexID()); 

      // ImGuiIO.DisplaySize 是通过 ImGui_ImplSDL3_NewFrame() 更新的 SDL 窗口逻辑尺寸。
      shader.bind({draw_data->DisplayPos.x,  draw_data->DisplayPos.y},
                  {draw_data->DisplaySize.x, draw_data->DisplaySize.y},
                  texture);

      render_state.scissor = scissor;

      // cmd.ElemCount 即当前 ImDrawCmd 的索引数量.
      Medium::Pipeline::IndexedDrawView draw_view{
        .vertices      = bd->vertices,
        .indices       = bd->indices,
        .index_offset  = index_base + cmd.IdxOffset,
        .index_count   = cmd.ElemCount,
        .vertex_offset = vertex_base + cmd.VtxOffset
      };

      Medium::Pipeline::draw(draw_view, render_state, viewport, framebuffer, bd->shader, nullptr);
    } // for(const ImDrawCmd& cmd)
  } // for(int i = 0; i < draw_data->CmdLists.Size)
}

#endif // #ifndef IMGUI_DISABLE
