///
/// @file   EnvironmentSH9.h
/// @author Garfield949
/// @date   2026/10/22 15:47
/// @note   2 阶 9 系数球谐函数 L(ω)，输入为一个世界空间单位向量。
///

#pragma once

#include <array>

#include "Base/Types.h"
#include "Base/Color.h"

namespace Medium {
class CubeMap;

/// @class EnvironmentSH9
/// @brief 存储经余弦卷积并预除Pi的漫反射环境光SH9系数。
class EnvironmentSH9 {
public:
  // Eigen::Vector3f 默认构造并不会初始化各分量，作为累加器需手动置零。
  explicit EnvironmentSH9() { for(auto& co: coefficients_) co.setZero(); }

  static EnvironmentSH9 FromCubeMap(const CubeMap& cube_map);

  LinearColor evaluate(const Vec3f& normal_world) const noexcept;

private:
  std::array<Vec3f, 9> coefficients_{};
};

} // namespace Medium
