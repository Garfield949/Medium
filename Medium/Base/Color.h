///
/// @file Color.h
/// @brief 颜色类型
///
#pragma once

#include <fmt/base.h>

#include "Base/Math/Math.h"
#include "Base/Util.h"

namespace Medium {

enum class ColorSpace {
  SRGB, Linear
};

// 标准 Gamma decoding : IEC 61966-2-1
// https://www.w3.org/Graphics/Color/srgb
inline static const std::array<f32, 256> GammaDecodingLUT = [] {
  std::array<f32, 256> lut{};
  constexpr f32 inv255 = 1.0f / 255.0f;
  for (size_t i = 0; i < 256; i++) {
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
inline static const std::array<u8, kGammaEncodeResolution + 1> GammaEncodingLUT = [] {
  std::array<u8, kGammaEncodeResolution + 1> lut{};
  for (size_t i = 0; i <= kGammaEncodeResolution; i++) {
    if (f32 linear = static_cast<f32>(i) / static_cast<f32>(kGammaEncodeResolution); linear <= 0.0031308f)
      lut[i] = Util::quantizeToU8(linear * 12.92f);
    else
      lut[i] = Util::quantizeToU8(1.055f * std::powf(linear, 0.4166667f) - 0.055f);
  }
  return lut;
}();

struct LinearColor;

/// @struct  Color
/// @brief   提供对 uint32_t 的包装，最终存储、交付的颜色类型，可能会包含 Gamma 矫正。
/// @details 内存布局：小端法 Little Endian：B G R A. 各通道占一个 u8.
struct Color {
  u32 data;
  // 构造函数，默认 alpha 为 255；
  // 采用显式转换，否则值会被隐式提升为 int，符号位为 1 时会变为负数（C++标准中，有符号数左移导致符号位翻转是未定义行为）；
  // 其实不转换也没事，因为 argb 是通过位模式与出来的。
  Color(u8 r, u8 g, u8 b, u8 a = 255)
    : data{(static_cast<u32>(a) << 24) | (static_cast<u32>(r) << 16) |
            (static_cast<u32>(g) << 8)  |  static_cast<u32>(b)} {}
    
  // 委托构造：灰度。
  Color(u8 grey_scale, u8 a = 255)
    : Color{grey_scale, grey_scale, grey_scale, a} {}

  Color() : Color{0} {}

  [[nodiscard]] LinearColor toLinear(ColorSpace space) const noexcept;

  [[nodiscard]] u8 a() const noexcept { return (data >> 24) & 0xFF; }
  [[nodiscard]] u8 r() const noexcept { return (data >> 16) & 0xFF; }
  [[nodiscard]] u8 g() const noexcept { return (data >>  8) & 0xFF; }
  [[nodiscard]] u8 b() const noexcept { return data & 0xFF; }

  static Color Black() noexcept { return {}; }
  static Color White() noexcept { return {255}; }
  static Color Red()   noexcept { return {255, 0, 0}; }
  static Color Green() noexcept { return {0, 255, 0}; }
  static Color Blue()  noexcept { return {0, 0, 255}; }
  static Color Navy()  noexcept { return {79, 147, 184}; }
}; // struct Color


/// @struct LinearColor
/// @brief  表示线性（物理）空间的颜色，r, g, b, a 为限定在 [0.0f, 1.0f] 范围内的浮点数。
struct LinearColor {
  f32 R = 0.0f, G = 0.0f, B = 0.0f, A = 1.0f;

  LinearColor() noexcept = default;
  
  LinearColor(f32 r, f32 g, f32 b, f32 a = 1.0f, ColorSpace space = ColorSpace::Linear) noexcept {
    if(space == ColorSpace::Linear) {
      R = r; G = g; B = b; A = a;
    } else {
      R = GammaDecodingLUT[Util::quantizeToU8(r)];
      G = GammaDecodingLUT[Util::quantizeToU8(g)];
      B = GammaDecodingLUT[Util::quantizeToU8(b)];
      A = a;
    }
  }
    
  LinearColor(f32 grey_scale, f32 a = 1.0f) noexcept
    : LinearColor(grey_scale, grey_scale, grey_scale, a) {}

  /// @brief 运算符重载
  /// @details 注意：+、- 这样的运算，Alpha 通道也跟着算。执行如 (A + B) - C 这样的
  ///          连续运算，不会丢失 HDR 的数据。
  LinearColor operator+(const LinearColor& rhs) const noexcept {
    return { R + rhs.R, G + rhs.G, B + rhs.B, A + rhs.A };
  }
  LinearColor operator-(const LinearColor& rhs) const noexcept {
    return { R - rhs.R, G - rhs.G, B - rhs.B, A - rhs.A };
  }
  LinearColor& operator+=(const LinearColor& rhs) noexcept {
    *this = *this + rhs;
    return *this;
  }
  LinearColor& operator-=(const LinearColor& rhs) noexcept {
    *this = *this - rhs;
    return *this;
  }
  LinearColor operator*(f32 scalar) const noexcept {
    return { R * scalar, G * scalar, B * scalar, A * scalar };
  }
  LinearColor& operator*=(f32 scalar) noexcept {
    *this = *this * scalar;
    return *this;
  }
  LinearColor operator*(const LinearColor& rhs) const noexcept {
    return { R * rhs.R, G * rhs.G, B * rhs.B, A * rhs.A };
  }
  LinearColor operator/(f32 scalar) const noexcept {
    f32 scalar_inv = 1.0f / scalar;
    return { R * scalar_inv, G * scalar_inv, B * scalar_inv, A * scalar_inv };
  }
  LinearColor& operator/=(f32 scalar) noexcept {
    *this = *this / scalar;
    return *this;
  }
  
  // 暂不考虑线性颜色某通道变为负数的情况
  [[nodiscard]] bool isBlack() const noexcept {
    return Math::isNearlyZero(R) && Math::isNearlyZero(G) && Math::isNearlyZero(B);
  }

  /// @brief Premultiplied alpha
  /// @details 关于 Alpha blending: 设某像素已有颜色为 Cdst, 待混合颜色为 Csrc, Cout 为混合输出颜色，则：
  ///          1. 不做预乘的混合公式：Cout = Csrc * Asrc + Cdst * (1 - Asrc).
  ///          2. 做了预乘的混合公式：Cout = Csrc + Cdst * (1 - Asrc), 会简单一些。
  [[nodiscard]] LinearColor premultiplied() const noexcept {
    return {R * A, G * A, B * A, A};
  }

  /// @brief Straight alpha
  /// @details 注意，若颜色的 Alpha = 0，则在 premul 阶段就会将 RGB 乘为 0，进而在 demul 阶段无法恢复
  ///          三原色分量。不过这是符合语义的，因为 Alpha = 0 代表完全透明。
  [[nodiscard]] LinearColor demultiplied() const noexcept {
    f32 alpha_clamped    = std::clamp(A, 0.0f, 1.0f);
    f32 alpha_linear_inv = Math::isNearlyZero(alpha_clamped) ? 0.0f : 1.0f / alpha_clamped;
    return {R * alpha_linear_inv, G * alpha_linear_inv, B * alpha_linear_inv, alpha_clamped};
  }
  
  static LinearColor premulOver(const LinearColor& src, const LinearColor& dst) noexcept {
    return src + dst * (1.0f - src.A);
  }

  /// @brief 将各分量钳制在 [0, 1]
  [[nodiscard]] LinearColor clamped() const noexcept {
    return {
      std::clamp(R, 0.0f, 1.0f),
      std::clamp(G, 0.0f, 1.0f),
      std::clamp(B, 0.0f, 1.0f),
      std::clamp(A, 0.0f, 1.0f)
    };
  }
  
  /// @brief ACES 色调映射简单拟合
  /// @details 将各分量的高动态范围压缩到 [0, 1] LDR 范围内。
  /// @note 严格来说，线性颜色经色调映射后就不再是线性的了，语义上偷了点懒。
  [[nodiscard]] LinearColor toneMapped() const noexcept {
    constexpr f32 a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
    auto formula = [&](f32 channel) {
      channel = std::max(0.0f, channel);
      return channel * (a * channel + b) / (channel * (c * channel + d) + e);
    };
    return {formula(R), formula(G), formula(B), A};
  }

  /// @brief 转换为 Color 定点数颜色类型。
  /// @param space 传入目标 Color 的预期色彩空间枚举
  ///              1. ColorSpace::SRGB:   进行伽马矫正再量化到 u8
  ///              2. ColorSpace::Linear: 线性直出并量化到 u8
  [[nodiscard]] Color toColor(ColorSpace space) const noexcept;

  static LinearColor Black() noexcept { return {}; }
  static LinearColor White() noexcept { return {1.0f}; }
  static LinearColor Red()   noexcept { return {1.0f, 0.0f, 0.0f}; }
  static LinearColor Green() noexcept { return {0.0f, 1.0f, 0.0f}; }
  static LinearColor Blue()  noexcept { return {0.0f, 0.0f, 1.0f}; }
  static LinearColor Navy()  noexcept { return {0.0761f, 0.2832f, 0.4678f}; }

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
      Util::quantizeToU8(tmp.A)
    };
  }
  // TODO: 线性直出，暂时没有应用场景
  return {
    Util::quantizeToU8(tmp.R),
    Util::quantizeToU8(tmp.G),
    Util::quantizeToU8(tmp.B),
    Util::quantizeToU8(tmp.A)
  };
}

} // namespace Medium