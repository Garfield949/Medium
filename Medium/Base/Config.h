///
/// @file   Config.h
/// @author Garfield949
/// @date   2026/3/1 22:34
/// @brief  存放应用程序和渲染相关的配置常量
///
#pragma once

#include "Base/Color.h"

namespace Medium::AppConstants {
  
  inline std::string WindowTitle = "Medium Canvas";
  
  inline u32 ViewportWidth  = 1440;
  inline u32 ViewportHeight = 900;
  
  inline f32 getScreenRatio() {
    return static_cast<f32>(ViewportWidth) / static_cast<f32>(ViewportHeight);
  }
  
} // namespace Medium::AppConstants

namespace Medium::RenderConstants {
  
  inline LinearColor FrameBufferClearColor = LinearColor::White();

  inline constexpr f32 DepthClearValue = 0.0f;
  
} // namespace Medium::RenderConstants