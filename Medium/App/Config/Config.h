///
/// @file   Config.h
/// @author Garfield949
/// @date   2026/3/1 22:34
/// @brief  
///
#pragma once

#include <filesystem>

#include "Constants.h"

namespace Medium {
  
struct WindowCfg {
  std::string title{Config::kDefaultWindowTitle};
  u32 w = Config::kDefaultWindowWidth;
  u32 h = Config::kDefaultWindowHeight;

  f32 aspectRatio() const {
    return static_cast<f32>(w) / static_cast<f32>(h);
  }
};

struct ShadowCfg {
  u32 shadow_map_res = Config::kDefaultShadowTileRes;
};

struct RenderCfg {
  ShadowCfg shadow{};
};

struct AppCfg {
  [[nodiscard]]
  static AppCfg LoadFromToml(const std::filesystem::path& config_dir);

  std::string toTomlString() const;
  
  AppCfg sanitized() const noexcept;

  void saveToTomlFile() const;

  WindowCfg window{};
  RenderCfg render{};

  std::filesystem::path related_toml_path;
};
  
} // namespace Medium