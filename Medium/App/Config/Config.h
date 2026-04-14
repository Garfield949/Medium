///
/// @file   Config.h
/// @author Garfield949
/// @date   2026/3/1 22:34
/// @brief  存放应用程序和渲染相关的配置常量
///
#pragma once

#include <filesystem>

#include "Constants.h"

namespace Medium {
  
  struct WindowCfg {
    std::string title{Config::kDefaultWindowTitle};
    u32 width  = Config::kDefaultWindowWidth;
    u32 height = Config::kDefaultWindowHeight;

    [[nodiscard]] f32 aspectRatio() const {
      return static_cast<f32>(width) / static_cast<f32>(height);
    }
  };
  
  struct ShadowCfg {
    u32 shadow_map_res = Config::kDefaultShadowRes;
    u32 csm_level = Config::kDefaultCSMLevel;
  };
  
  struct RenderCfg {
    ShadowCfg shadow{};
  };
  
  struct AppCfg {
    static AppCfg loadFromToml(const std::filesystem::path& config_dir);

    [[nodiscard]] std::string toTomlString() const noexcept;
    
    [[nodiscard]] AppCfg sanitized() const noexcept;

    void saveToTomlFile() const;

    WindowCfg window{};
    RenderCfg render{};

    std::filesystem::path related_toml_path;
  };
  
} // namespace Medium