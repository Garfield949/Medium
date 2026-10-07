///
/// @file Color.h
/// @brief 颜色类型
///
#pragma once

#include <algorithm>
#include <type_traits> // std::is_trivially_copyable_v<T>

#include "Base/Math/Math.h"

namespace Medium {

inline u8 quantizeToU8(f32 value) {
  return static_cast<u8>(std::lround(value * 255.0f));
}

enum class ColorSpace {
  SRGB, Linear
};

// 标准 Gamma decoding : IEC 61966-2-1
// https://www.w3.org/Graphics/Color/srgb
inline const std::array<f32, 256> GammaDecodingLUT = [] {
  std::array<f32, 256> lut;
  constexpr f32 inv255 = 1.0f / 255.0f;
  for (size_t i = 0; i < 256; ++i) {
    if (const f32 channel = static_cast<f32>(i) * inv255; channel <= 0.04045f)
      lut[i] = channel / 12.92f;
    else
      lut[i] = std::powf((channel + 0.055f) / 1.055f, 2.4f);
  }
  return lut;
}();

// 标准 Gamma encoding : IEC 61966-2-1
// https://www.w3.org/Graphics/Color/srgb
// 注意表大小得是分辨率 + 1. 否则索引 i 范围为 [0, 4095]，最大只能计算 4095/4096，取不到 1.0 的 Linear 值
inline constexpr size_t kGammaEncodeResolution = 4096;
inline const std::array<u8, kGammaEncodeResolution + 1> GammaEncodingLUT = [] {
  std::array<u8, kGammaEncodeResolution + 1> lut;
  for (size_t i = 0; i <= kGammaEncodeResolution; ++i) {
    if (f32 linear = static_cast<f32>(i) / static_cast<f32>(kGammaEncodeResolution); linear <= 0.0031308f)
      lut[i] = quantizeToU8(linear * 12.92f);
    else
      lut[i] = quantizeToU8(1.055f * std::powf(linear, 0.4166667f) - 0.055f);
  }
  return lut;
}();

struct LinearColor;

/// @struct  Color
/// @brief   提供对 ABGR8 的包装。最终存储、交付的颜色为 SDL uint32_t ABGR8，可能包含 Gamma 矫正。
/// @details 小端 Little Endian 内存布局示意：低地址 -> RR GG BB AA -> 高地址. 各通道占一个 u8.
/// @note    采用该布局的原因：stbi 读入的纹理指针 unsigned char* 指向的内存块即为低地址到高地址平铺的 RGBA，每个像素可直接无开销
///          转型为一个 Color.
struct Color {
  // u8 左移会被隐式提升为一个 int. 比方说：
  //                                  u8 a = 255; u32 b = a << 24;
  // a 的位模式为 0xFF，提升后变成 (int)0x0000 00FF. 这里的坑是，左移 24 位会恰好使符号位翻转：0xFF00 0000，这是 UB. 虽然后续位
  // 模式 0xFF00 0000 还是大概率会被正确解释为一个 u32，但 UB 就让人很不爽。所以这里显式地提升为一个 u32，就不存在符号位这个东西，也
  // 就没有 UB 这个说法了。
  constexpr Color(u8 r, u8 g, u8 b, u8 a = 255)
    : data{(static_cast<u32>(a) << 24) | (static_cast<u32>(b) << 16) |
           (static_cast<u32>(g) << 8)  |  static_cast<u32>(r)} {}
    
  // 灰度委托构造。
  constexpr Color(u8 grey_scale, u8 a = 255)
    : Color{grey_scale, grey_scale, grey_scale, a} {}

  constexpr Color() : Color{0} {}

  constexpr u8 a() const noexcept { return (data >> 24) & 0xFF; }
  constexpr u8 b() const noexcept { return (data >> 16) & 0xFF; }
  constexpr u8 g() const noexcept { return (data >>  8) & 0xFF; }
  constexpr u8 r() const noexcept { return data & 0xFF; }

  static constexpr Color Black() noexcept { return {}; }
  static constexpr Color White() noexcept { return {255}; }
  static constexpr Color Red()   noexcept { return {255, 0, 0}; }
  static constexpr Color Green() noexcept { return {0, 255, 0}; }
  static constexpr Color Blue()  noexcept { return {0, 0, 255}; }
  static constexpr Color Navy()  noexcept { return {79, 147, 184}; }

  [[nodiscard]]
  LinearColor toLinear(ColorSpace space) const noexcept;

  // 暂时没用。
  [[nodiscard]]
  u32 toPackedArgb8() const noexcept {
    // 保留 A_G_ | 将 B 移到最低位 | 将 R 移到次高位。
    return (data & 0xFF00FF00u) | ((data & 0x00FF0000u) >> 16) | ((data & 0x000000FFu) << 16);
  }

  u32 data;
}; // struct Color


/// @struct LinearColor
/// @brief  表示线性（物理）空间的颜色。
struct LinearColor {
  constexpr LinearColor() noexcept = default;

  constexpr LinearColor(f32 r, f32 g, f32 b, f32 a = 1.0f) noexcept
    : R{r}, G{g}, B{b}, A{a} {}
  
  constexpr LinearColor(f32 grey_scale, f32 a = 1.0f) noexcept
    : LinearColor{grey_scale, grey_scale, grey_scale, a} {}
  
  static LinearColor FromNormalizedSRGB(f32 r, f32 g, f32 b, f32 a = 1.0f) noexcept {
    LinearColor res;
    res.R = GammaDecodingLUT[quantizeToU8(r)];
    res.G = GammaDecodingLUT[quantizeToU8(g)];
    res.B = GammaDecodingLUT[quantizeToU8(b)];
    res.A = a;
    return res;
  }

  bool isBlack() const noexcept {
    return Math::isNearlyZero(R) && Math::isNearlyZero(G) && Math::isNearlyZero(B);
  }

  /// @brief 运算符重载
  /// @details 注意：+、- 这样的运算，Alpha 通道也跟着算。执行如 (A + B) - C 这样的
  ///          连续运算，不会丢失 HDR 的数据。
  constexpr LinearColor operator+(const LinearColor& rhs) const noexcept {
    return { R + rhs.R, G + rhs.G, B + rhs.B, A + rhs.A };
  }
  
  constexpr LinearColor operator-(const LinearColor& rhs) const noexcept {
    return { R - rhs.R, G - rhs.G, B - rhs.B, A - rhs.A };
  }
  
  constexpr LinearColor& operator+=(const LinearColor& rhs) noexcept {
    R += rhs.R; 
    G += rhs.G; 
    B += rhs.B; 
    A += rhs.A;
    return *this;
  }
  
  constexpr LinearColor& operator-=(const LinearColor& rhs) noexcept {
    R -= rhs.R; 
    G -= rhs.G; 
    B -= rhs.B; 
    A -= rhs.A;
    return *this;
  }
  
  constexpr LinearColor operator*(const LinearColor& rhs) const noexcept {
    return { R * rhs.R, G * rhs.G, B * rhs.B, A * rhs.A };
  }
  
  constexpr LinearColor operator*(f32 scalar) const noexcept {
    return { R * scalar, G * scalar, B * scalar, A * scalar };
  }
  
  constexpr LinearColor& operator*=(f32 scalar) noexcept {
    R *= scalar; 
    G *= scalar; 
    B *= scalar; 
    A *= scalar;
    return *this;
  }
  
  constexpr LinearColor operator/(f32 scalar) const noexcept {
    f32 scalar_inv = 1.0f / scalar;
    return { R * scalar_inv, G * scalar_inv, B * scalar_inv, A * scalar_inv };
  }
  
  constexpr LinearColor& operator/=(f32 scalar) noexcept {
    f32 scalar_inv = 1.0f / scalar;
    R *= scalar_inv; 
    G *= scalar_inv; 
    B *= scalar_inv; 
    A *= scalar_inv;
    return *this;
  }

  /// @brief 将各分量钳制在 [0, 1]
  constexpr LinearColor clamped() const noexcept {
    return {
      std::clamp(R, 0.0f, 1.0f),
      std::clamp(G, 0.0f, 1.0f),
      std::clamp(B, 0.0f, 1.0f),
      std::clamp(A, 0.0f, 1.0f)
    };
  }

  /// @brief Premultiplied alpha
  /// @details 关于 Alpha blending: 设某像素已有颜色为 Cdst, 待混合颜色为 Csrc, Cout 为混合输出颜色，则：
  ///          1. 不做预乘的混合公式：Cout = Csrc * Asrc + Cdst * (1 - Asrc).
  ///          2. 做了预乘的混合公式：Cout = Csrc + Cdst * (1 - Asrc), 会简单一些。
  constexpr LinearColor premultiplied() const noexcept {
    return {R * A, G * A, B * A, A};
  }

  /// @brief Straight alpha
  /// @details 注意，若颜色的 Alpha = 0，则在 premul 阶段就会将 RGB 乘为 0，进而在 demul 阶段无法恢复
  ///          三原色分量。不过这是符合语义的，因为 Alpha = 0 代表完全透明。
  LinearColor demultiplied() const noexcept {
    f32 alpha_clamped    = std::clamp(A, 0.0f, 1.0f);
    f32 alpha_linear_inv = Math::isNearlyZero(alpha_clamped) ? 0.0f : 1.0f / alpha_clamped;
    return {R * alpha_linear_inv, G * alpha_linear_inv, B * alpha_linear_inv, alpha_clamped};
  }

  /// @brief 预乘叠加混合。
  /// @details src 为新生成颜色，dst 为着色点已存在的背景色。
  static constexpr LinearColor PremulOver(const LinearColor& src_premult, const LinearColor& dst) noexcept {
    return src_premult + dst * (1.0f - src_premult.A);
  }

  /// @brief 转换为 Color 定点数颜色类型。
  /// @param space 传入目标 Color 的预期色彩空间枚举
  ///              1. ColorSpace::SRGB:   进行伽马矫正再量化到 u8
  ///              2. ColorSpace::Linear: 线性直出并量化到 u8
  Color toColor(ColorSpace space) const noexcept;

  static constexpr LinearColor Black() noexcept { return {}; }
  static constexpr LinearColor White() noexcept { return {1.0f}; }
  static constexpr LinearColor Red()   noexcept { return {1.0f, 0.0f, 0.0f}; }
  static constexpr LinearColor Pink()  noexcept { return {1.0, 0.078, 0.576}; }
  static constexpr LinearColor Green() noexcept { return {0.0f, 1.0f, 0.0f}; }
  static constexpr LinearColor Blue()  noexcept { return {0.0f, 0.0f, 1.0f}; }
  static constexpr LinearColor Navy()  noexcept { return {0.0761f, 0.2832f, 0.4678f}; }

  auto data(this auto& self) noexcept {
    return &self.R;
  }

  f32 R = 0.0f, G = 0.0f, B = 0.0f, A = 1.0f;
}; // struct LinearColor

inline LinearColor Color::toLinear(ColorSpace space) const noexcept {
  f32 inv255 = 1.0f / 255.0f;
  f32 alpha_linear = static_cast<f32>(a()) * inv255;
  if(space == ColorSpace::SRGB)
    return {
      GammaDecodingLUT[r()],
      GammaDecodingLUT[g()],
      GammaDecodingLUT[b()],
      alpha_linear
    };
  return {
    static_cast<f32>(r()) * inv255,
    static_cast<f32>(g()) * inv255,
    static_cast<f32>(b()) * inv255,
    alpha_linear
  };
}

inline Color LinearColor::toColor(ColorSpace space) const noexcept {
  LinearColor tmp = this->clamped();
  if(space == ColorSpace::SRGB) {
    auto IndexLUT = [&](f32 linear) -> size_t {
      return std::lround(linear * static_cast<f32>(kGammaEncodeResolution));
    };
    return {
      GammaEncodingLUT[IndexLUT(tmp.R)],
      GammaEncodingLUT[IndexLUT(tmp.G)],
      GammaEncodingLUT[IndexLUT(tmp.B)],
      quantizeToU8(tmp.A)
    };
  }
  return {
    quantizeToU8(tmp.R),
    quantizeToU8(tmp.G),
    quantizeToU8(tmp.B),
    quantizeToU8(tmp.A)
  };
}

static_assert(sizeof(LinearColor) == sizeof(f32) * 4);
static_assert(std::is_trivially_copyable_v<LinearColor>); // 确认是否为可按字节安全搬运的类型。

static_assert(sizeof(Color) == sizeof(u32));
static_assert(std::is_trivially_copyable_v<Color>); // 确认是否为可按字节安全搬运的类型。

} // namespace Medium