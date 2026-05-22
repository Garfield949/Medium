///
/// @file   DirectionalLight.cpp
/// @author Garfield949
/// @date   2026/5/2 22:11
/// @brief  
///
#include "DirectionalLight.h"

#include "Pipeline/Shadow/DirectionalViewFit.h"

namespace Medium {

void DirectionalLight::applyViewFit(const DirectionalViewFit& fit) {
  transform.setPosition(fit.light_pos_ws);
  setOrthoParams(fit.ortho_near, fit.ortho_far, fit.ortho_width, fit.ortho_height);
}

} // namespace Medium
