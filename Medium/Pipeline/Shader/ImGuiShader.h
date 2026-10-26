///
/// @file   ImGuiShader.h
/// @author Garfield949
/// @date   2026/6/19 15:44
/// @brief  ImGui 绘制命令走的着色器，负责将 ImGui 吐出的 2D 顶点 (pos, uv, color) 转为 Medium 可消费
///         的形式。
///
#pragma once

#include "Base/Color.h"
#include "Base/Types.h"
#include "Resources/Vertex.h"
#include "Resources/Material.h"
#include "Resources/Sampler.h"
#include "Pipeline/RenderContext.h"
#include "Pipeline/Shader/ShaderTypes.h"

namespace Medium {

struct ImGuiShader {
  struct Uniform {
    Vec2f display_pos;                        // ImGui 当前 ImDrawCmd 的视口左上角原点，一般是 (0,0).
    Vec2f display_size;                       // ImGui 当前 ImDrawCmd 的视口逻辑尺寸；这两项一般就是 SDL 窗口的左上角和尺寸。
    const Image2D<Color>* texture = nullptr;  // ImGui 当前 ImDrawCmd 使用的贴图（若是 UI 文本，则是一个 font atlas）。
  };

  static constexpr i32 kOffsetColor        = kOffsetUV    + 2;
  static constexpr i32 kVaryingScalarCount = kOffsetColor + 4;

  // Dummy.
  void bind(const ShaderBindContext&, const Mat4f&, const Material*) noexcept {}

  void bind(Vec2f display_pos, Vec2f display_size, const Image2D<Color>* texture) noexcept;

  [[nodiscard]] // 直接输出一个 NDC.
  VSOut vertex(const Vertex& vert) const noexcept {
    const auto& [display_pos, display_size, texture] = uniform;
    // Vertex 对 ImDrawVert 进行封装：只存 2D 屏幕 xy 坐标，z = 0.
    const f32 w = display_size.x();
    const f32 h = display_size.y();
    // 将 ImGui 传入的屏幕坐标映射为 NDC (减去视口原点变为一个相对坐标)；
    // ImGui 的屏幕坐标定义为左上角，所以 y 的映射需要翻转一下。
    const f32 x_relative = vert.pos.x() - display_pos.x();
    const f32 y_relative = vert.pos.y() - display_pos.y();
    const f32 x_ndc = x_relative * 2.0f / w - 1.0f;
    const f32 y_ndc = 1.0f - y_relative * 2.0f / h;
    
    VSOut vs_out;
    vs_out.pos_clip = {x_ndc, y_ndc, 0.5f, 1.0f}; // 走透视投影插值，w 设为 1 即退化为线性插值。
    vs_out.varying.store(vert.uv.data()   , kOffsetUV   , 2);
    vs_out.varying.store(vert.color.data(), kOffsetColor, 4);
    return vs_out;
  }

  [[nodiscard]]
  bool fragment(const FSIn& fs_in, LinearColor& frag_color) const noexcept {
    const Vec2f uv = fs_in.getVec2f(kOffsetUV);
    const LinearColor vert_col = fs_in.getLinearColor(kOffsetColor);

    LinearColor texel_col{1.0f};
    if(uniform.texture)
      // ImGui 强烈建议用双线性采样。
      texel_col = Samplers::LinearClamp.sample(*uniform.texture, ColorSpace::SRGB, uv);

    // ImGui 的像素颜色 = 顶点颜色 * 材质颜色。
    frag_color = texel_col * vert_col;
    // 透明的直接丢掉。
    return frag_color.A == 0.0f;
  }

  Uniform uniform;
};

} // namespace Medium
