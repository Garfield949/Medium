///
/// @file   TextureSlot.h
/// @author Garfield949
/// @date   2026/3/4 23:44
/// @brief  使用 std::variant 的精髓在于，着色器并不知道当前 SubMesh 的材质到底是要在贴图上采样
///         还是直接用从 MTL 读出的纯色。只需提前决策好材质槽里到底装的是啥，采样颜色时动态判断即可。
///
#pragma once

#include "Forwards.h"
#include "Texture.h"

namespace Medium {
  
using TextureSlot = std::variant<TexturePtr, LinearColor>;

// loadTex 返回一个右值，所以直接移动
inline TextureSlot makeSlot(TexturePtr map, const LinearColor& soild_color) {
  if(map) return std::move(map);
  return soild_color;
}

inline LinearColor sampleSlot(const TextureSlot& slot, const Vec2f& uv) {
    // 0 对应 TexturePtr, 1 对应 LinearColor
    if (slot.index() == 0) {
        const auto& map = std::get<0>(slot);
        return map->sample(uv, Texture::FilterMode::Bilinear);
    }
    return std::get<1>(slot);
}
  
}
