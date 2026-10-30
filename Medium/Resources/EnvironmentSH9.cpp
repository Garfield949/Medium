///
/// @file   EnvironmentSH9.cpp
/// @author Garfield949
/// @date   2026/10/22 15:47
/// @brief  
/// @note   1. 给定方向 ω, 可以从 Cubemap 采样方向所指的环境颜色. 而 2 阶 9 系数球谐函数通过拟合的方式，可得到 ω 所对应的的低频近似.
///            如果只想求 L(ω), 直接采样 Cubemap 最精确。但由于物体表面点会同时接收周围各种方向的光，故环境光需计算着色点法线半球内所有方向的积分 E(n). 而 SH9 可用很低的成本来实现。
///            注意：(1) 辐亮度, Radiance   指「单位投影面积 / 立体角内的辐射能量」，与特定方向相关，即 L(ω).
///                 (2) 辐照度, Irradiance 是单位面积所有方向上入射光的积分总和，非特定方向的量，即 E(n).
///            辐照度 E(n) = ∫ L(ω)max(0,n·ω)dω 是对整个法线球面的积分，但背面入射光无贡献，所以用一个 max 排除，退化为半球积分。cosθ = n·ω, 故此积分叫做截断余弦卷积。
///         2. SH9 由 9 个球谐基函数组成，2 阶是指基函数的阶数包含 l = 0,1,2, 表示基函数的角频率等级，即其在球面上随方向变化的复杂程度，分别是：
///            l = 0:  常数 1, 表示球面上不随方向变化的部分；
///            l = 1:  x, y, z, 表示球面上随三个轴正负线性变化的部分；
///            l = 2:  xy, yz, xz, 3z^2-1, x^2-y^2，表示球面上随方向变化更复杂的部分。
///            注意：(1) 基函数的由来：来自于更深层数学概念---它们是球面拉普拉斯算子的特征函数，只需要了解即可，不需深究；
///                 (2) 特点：9 个基函数已经两两正交，即 ∫_S^2(Ω) f(ω)g(ω)dω = 0; 为了组成 SH9, 还要求它们与自身的内积为 1, 即 ∫ [Cf(ω)]^2dω = 1. 所以需将这些系数 C 求出来。
///         3. 以常数 1 为例，假设它的系数为 C, 则有 ∫ C^2dω = C^2·4π = 1, 解得 C = 1 / √4π ≈ 0.282094792.
///            又以 x 举例，假设其系数为 C, 则有 ∫ (Cx)^2dω = C^2 ∫ x^2dω = 1.
///            单位球面方程为 x^2+y^2+z^2=1, 根据对称性 C^2 ∫ x^2dω = C^2 · 1/3 · ∫ (x^2+y^2+z^2)dω = C^2 · 4π/3 = 1, 解得 C = √3/4π ≈ 0.488602512. y, z 及剩下的 2
///            阶基函数系数的求法同理。
///         4. 正式计算 SH9: L(ω) ≈ SH9 = ∑ CiYi(ω), 其中 Yi(ω) 即上述已得到归一化系数的 9 个球谐基函数。要将 Cubemap 拟合为 SH9, 需求出各 Ci. 假设现在要求 Cj (j = 0 ~ 8):
///            等式两边同时乘以 Yj(ω)
///                                     L(ω)Yj(ω) = ∑ CiYi(ω)Yj(ω)
///            两边同时对整个球面积分 (注：ω 作为参数代表方向，dω 代表该方向附近的立体角微元，但更严谨的是dΩ, 表示积分区域)
///                                     ∫ L(ω)Yj(ω)dω = ∫ ∑ CiYi(ω)Yj(ω)dω
///            由于基函数的特性，等式右边即 = Cj. 故得到系数的计算式子 Ci = ∫ L(ω)Yi(ω)dω.
///         5. 计算辐照度 E(n): 实际计算光照时，不可能为每个着色点算一遍余弦卷积，否则性能开销巨大。令余弦核函数 K(n·ω) = max(0,n·ω), 则有
///                                     E(n) = ∫ L(ω)max(0,n·ω)dω ≈ ∫ ∑ CiYi(ω)K(n·ω)dω = ∑ Ci ∫ Yi(ω)K(n·ω)dω
///            根据 Funk–Hecke 定理，对于仅依赖两个夹角的核函数 K(n·ω), 有
///                                     ∫ Yi(ω)K(n·ω)dω = A·Yi(n)
///            即对基函数和核函数乘积的球面积分可简单地用乘一个常数系数来代替。不过这些系数 A 需用更底层的数学工具来计算，这里只需知道 E(n) 可通过为 L(ω) 的每个基函数再乘一个 A 得到。
///            各阶数对应基函数的卷积系数 A 分别为: l = 0: π, l = 1: 2π/3, l = 2: π/4, 即得到 Esh9(n).
///         6. Albedo 表示「总共反射多少比例的入射能量」, Lambert BRDF 是对各方向的反射分配。而 Lambert 漫反射模型假设物体表面总是向各方向均匀反射光线，所以其 BRDF fr 是一个常数。
///            所以法线方向的半球出射能量 albedo = ρ = ∫ fr·cos0dω = fr ∫ cos0dω. cos0 的球面积分 = π, 故 fr·π = ρ -> fr = ρ / π.
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