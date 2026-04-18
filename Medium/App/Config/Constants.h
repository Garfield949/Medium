///
/// @file   Constants.h
/// @author Garfield949
/// @date   2026/4/12 22:35
/// @brief  
///
#pragma once

#include "Base/Types.h"

namespace Medium::Config {

inline constexpr std::string_view kConfigDirName  = "config";
inline constexpr std::string_view kConfigFileName = "app_config.toml";

inline constexpr std::string_view kDefaultWindowTitle = "Medium Canvas";

inline constexpr u32 kDefaultWindowWidth = 1440u;
inline constexpr u32 kMinWindowWidth     = 300u;

inline constexpr u32 kDefaultWindowHeight = 900u;
inline constexpr u32 kMinWindowHeight     = 300u;

inline constexpr u32 kDefaultShadowRes = 1024u;
inline constexpr u32 kMinShadowRes     = 512u;

inline constexpr u32 kDefaultCSMLevel = 2u;
inline constexpr u32 kMinCSMLevel     = 1u;
inline constexpr u32 kMaxCSMLevel     = 4u;
  
}
