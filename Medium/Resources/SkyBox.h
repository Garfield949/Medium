///
/// @file   SkyBox.h
/// @author Garfield949
/// @date   2026/10/24 14:42
/// @brief  
///
#pragma once

#include "Resources/CubeMap.h"
#include "Resources/EnvironmentSH9.h"

namespace Medium {

/// @struct  SkyBox
/// @details 天空盒类型。
struct SkyBox {
  CubeMap        cubemap;
  EnvironmentSH9 env_sh9;
};

} // namespace Medium
