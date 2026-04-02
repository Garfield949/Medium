///
/// @file   Config.h
/// @author Garfield949
/// @date   2026/3/1 22:34
/// @brief  存放应用程序和渲染相关的配置常量
///
#pragma once

#include <filesystem>

#include "Base/Types.h"

namespace Medium {
  
  struct WindowCfg {
    std::string title;
    u32 width;
    u32 height;

    [[nodiscard]] f32 aspectRatio() const {
      return static_cast<f32>(width) / static_cast<f32>(height);
    }
  };
  
  struct RenderCfg {
    u32 shadow_map_res;
  };
  
  struct AppCfg {
    WindowCfg window{};
    RenderCfg render{};
  };
  
  [[nodiscard]] AppCfg loadAppConfig(const std::filesystem::path& toml_dir);
  
} // namespace Medium