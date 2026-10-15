///
/// @file   TextureSlot.h
/// @author Garfield949
/// @date   2026/3/4 23:44
/// @brief  使用 std::variant 的精髓在于，着色器并不知道当前 SubMesh 的材质到底是要在贴图上采样
///         还是直接用从 MTL 读出的纯色。只需提前决策好材质槽里到底装的是啥，采样颜色时动态判断即可。
///
#pragma once

#include <variant>

#include "Sampler.h"

namespace Medium {
  class Texture2D;
}

namespace Medium {

using TextureSlot = std::variant<Texture2D*, LinearColor>;

inline TextureSlot makeSlot(Texture2D* map, const LinearColor& solid_color) {
  if(map) return map;
  return solid_color;
}

inline LinearColor sampleSlot(const TextureSlot& slot, const Vec2f& uv, const Vec2f& duv_dx, const Vec2f& duv_dy, bool mipmaping) {
    // 0 对应 TexturePtr, 1 对应 LinearColor
    if (slot.index() == 0) {
      // get 返回一个右值，必须绑定 const 引用。
      // 对材质直接使用 mip 采样。
      const Texture2D* texture = std::get<0>(slot);
      return mipmaping ? Samplers::TrilinearRepeat.sample(*texture, uv, duv_dx, duv_dy)
                       : Samplers::LinearRepeat.sample(texture->mipLevel(0), texture->colorSpace(), uv);
    }
    return std::get<1>(slot);
}

}
