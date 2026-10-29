///
/// @file   EnvironmentSH9.cpp
/// @author Garfield949
/// @date   2026/10/22 15:47
/// @brief  
///
#include "EnvironmentSH9.h"

#include "Base/Color.h"
#include "Base/Types.h"
#include "Resources/CubeMap.h"

namespace Medium {
namespace {
/// @brief 获取未经余弦卷积的球谐函数 Y(ω) 系数。
std::array<f32, 9> getSH9Basis(const Vec3f& w_world) {
  const f32 x = w_world.x(), y = w_world.y(), z = w_world.z();
  constexpr f32 kL0   = 0.282094792f;                                              // 阶 l=0 系数
  constexpr f32 kL1   = 0.488602512f;                                              // 阶 l=1 系数
  constexpr f32 kL2_0 = 1.092548431f, kL2_1 = 0.315391565f, kL2_2 = 0.546274215f;  // 阶 l=2 系数
  return {
    kL0,
    kL1 * x, 
    kL1 * y, 
    kL1 * z,
    kL2_0 * x * y, 
    kL2_0 * y * z, 
    kL2_0 * x * z,
    kL2_1 * (3.0f * z * z - 1.0f),
    kL2_2 * (x * x - y * y)
  };
}

/// @brief 计算立体角所需的原函数值。
f64 calcAntiderivativeVal(f64 s, f64 t) {
  return std::atan2(
    s * t,
    std::sqrt(1 + s * s + t * t)
  );
}

/// @brief 计算并返回当前像素的立体角。
/// @note  球面积分微元 dω = sinθdθdφ, 转换为球坐标变为 dω = dsdt / (1 + s^2 + t^2)^(3/2).
///        计算单像素的立体角即在像素范围内对 dω 进行二重积分。而 dω 具有原函数
///                    F(s,t) = arctan[st/sqrt(1+s^2+t^2)]
///        所以利用牛顿-莱布尼茨公式计算 Δω = F(s1,t1)-F(s0,t1)-F(s1,t0)+F(s0,t0).
f64 calcSolidAngle(f64 s0, f64 t0, f64 s1, f64 t1) {
  return calcAntiderivativeVal(s1, t1) - calcAntiderivativeVal(s0, t1)
       - calcAntiderivativeVal(s1, t0) + calcAntiderivativeVal(s0, t0);
}

/// @brief 将面内归一化坐标转换为世界空间向量（需进行空间想象）。
Vec3f calcWorldDir(CubeFace face, f32 s, f32 t) {
  switch(face) {
  case CubeFace::PosX: return Vec3f{ 1.0f,  t,    -s}.normalized();
  case CubeFace::NegX: return Vec3f{-1.0f,  t,     s}.normalized();
  case CubeFace::PosY: return Vec3f{ s,  1.0f,    -t}.normalized();
  case CubeFace::NegY: return Vec3f{ s, -1.0f,     t}.normalized();
  case CubeFace::PosZ: return Vec3f{ s,     t,  1.0f}.normalized();
  case CubeFace::NegZ: return Vec3f{-s,     t, -1.0f}.normalized();
  }
  // C++23, 提供优化假设。
  std::unreachable();
}
  
} // 匿名命名空间，控制仅单文件可见


EnvironmentSH9 EnvironmentSH9::FromCubeMap(const CubeMap &cube_map) {
  // Cubemap 可能总共有成百上千万像素，所以用双精度累加，结果转单精度。
  std::array<Vec3d, 9> coeff_acc{};
  for(auto& coeff: coeff_acc) coeff.setZero();
  
  const i32 size = cube_map.resolution();
  const f64 scale = 2.0f / static_cast<f64>(size);

  for(i32 y = 0; y < size; ++y) {
    const f64 t0 = -1.0f + scale * static_cast<f64>(y);
    const f64 t1 = -1.0f + scale * static_cast<f64>(y + 1);
    
    for(i32 x = 0; x < size; ++x) {
      const f64 s0 = -1.0f + scale * static_cast<f64>(x);
      const f64 s1 = -1.0f + scale * static_cast<f64>(x + 1);
      const f64 solid_angle = calcSolidAngle(s0, t0, s1, t1);

      // 获取像素中心。
      const f32 s_center = static_cast<f32>(0.5f * (s0 + s1));
      const f32 t_center = static_cast<f32>(0.5f * (t0 + t1));
      
      // cubemap 六个面同位置像素的立体角一致，遍历累加即可。
      for(size_t face_idx = 0; face_idx < 6; ++face_idx) {        
        const auto face_select = static_cast<CubeFace>(face_idx);

        const Vec3f dir_world = calcWorldDir(face_select, s_center, t_center);

        const LinearColor linear = cube_map.face(face_select)[x,y].toLinear(ColorSpace::SRGB);
        
        const Vec3d Lw{linear.R, linear.G, linear.B};
        
        const auto sh9_basis = getSH9Basis(dir_world);

        // ci = Σ L(ω) * Y_i(ω) * dω
        for(size_t i = 0; i < 9; ++i)
          coeff_acc[i] += Lw * static_cast<f64>(sh9_basis[i]) * solid_angle;
      }
    }
  }

  // Lambert 的 BRDF 出射系数 fr = 漫反射系数 / Pi, 而下面要使 L(ω) -> E(ω) 的系数其实分子上有一个 Pi.
  // 最终计算漫反射时，相乘会抵消掉 Pi, 所以这里直接省略掉了。
  constexpr std::array<f64, 9> factors{
    1.0,
    2.0 / 3.0, 2.0 / 3.0, 2.0 / 3.0,
    1.0 / 4.0, 1.0 / 4.0, 1.0 / 4.0,
    1.0 / 4.0, 1.0 / 4.0
  };

  EnvironmentSH9 res;

  for(size_t i = 0; i < 9; ++i) 
    res.coefficients_[i] = (factors[i] * coeff_acc[i]).cast<f32>();
  
  return res;
}

LinearColor EnvironmentSH9::evaluate(const Vec3f& normal_world) const noexcept {
  const auto sh9_basis = getSH9Basis(normal_world);

  Vec3f rgb = Vec3f::Zero();

  // Σcoefficient_i * Y_i(n), 其实就是 E(n)/Pi.
  for(size_t i = 0; i < 9; ++i)
    rgb += coefficients_[i] * sh9_basis[i];

  // 可能有负值，截断一下。
  rgb = rgb.cwiseMax(0.0f);

  return {rgb.x(), rgb.y(), rgb.z(), 1.0f};
}
  
} // namespace Medium