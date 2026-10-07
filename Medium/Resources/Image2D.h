///
/// @file   Image2D.h
/// @author Garfield949
/// @date   2026/9/5 22:40
/// @brief  
///
#pragma once

#include <cassert>
#include <vector>
#include <concepts>

#include "Base/Types.h"
#include "Base/Color.h"

namespace Medium {

template<typename T>
concept PixelType = std::same_as<T, f32>   ||      // Depth Buffer.
                    std::same_as<T, Color> ||      // Present Buffer.
                    std::same_as<T, LinearColor>;  // HDR Buffer.

template<PixelType T>
class Image2D {
public:
  // 禁用拷贝构造和拷贝赋值，防止无意识复制行为；
  // 保留移动构造和移动赋值：
  // - Image2D image = LoadImage(path);  <-- 返回右值；
  // - Image2D image2{std::move(image)};
  Image2D(const Image2D&) = delete;
  Image2D& operator=(const Image2D&) = delete;
  // 编译器的奇怪特性：若显式定义了
  // - 拷贝构造；
  // - 移动构造；
  // - 拷贝赋值；
  // - 移动赋值；
  // - 析构函数;
  // 这 5 个中的至少 1 个，那么其它 4 个不会被隐式生成，必须手动定义，因为编译器认为我主动介入了该类型的赋值行为。
  Image2D(Image2D&&) noexcept = default;
  Image2D& operator=(Image2D&&) noexcept = default;

  explicit Image2D() = default;
  explicit Image2D(i32 w, i32 h): w_{w}, h_{h}, pixels_(w * h) {}
  
  i32 width()  const noexcept { return w_; }
  i32 height() const noexcept { return h_; }

  f32 widthF()  const noexcept { return static_cast<f32>(w_); }
  f32 heightF() const noexcept { return static_cast<f32>(h_); }

  size_t pixelCount() const noexcept { return pixels_.size(); }

  // Medium 在 读入模型 -> NDC -> 视口变换 链路上全程采用原点在左下角的坐标系。
  size_t index(i32 x, i32 y) const noexcept {
    assert(x >= 0 && x < w_ && y >= 0 && y < h_);
    return y * w_ + x;
  }

  void resize(i32 width, i32 height) {
    assert(width > 0 && height > 0);
    pixels_.resize(width * height);
    w_ = width;
    h_ = height;
  }

  void resize(i32 resolution) { resize(resolution, resolution); }

  void fill(const T& value) {
    std::fill(pixels_.begin(), pixels_.end(), value);
  }

  // C++23 Explicit object parameter, 显式对象参数，也称 deducing this，允许将 this 显式写进参数列表。
  // 1. 将 self 的类型标为 auto, 则 operator[] 根据调用对象自动推导 this 是否为 const.
  // 2. this auto& self 使用引用表示该函数操作的就是调用对象本身，不会造成拷贝。
  // 3. decltype(auto) 自动推导返回类型是否为 const. self.pixels_[i] 本身是一个引用表达式，所以返回值会推导为 T& 或 const T&.
  decltype(auto) operator[](this auto& self, size_t i) {
    assert(i < self.pixels_.size());
    return self.pixels_[i];
  }

  // C++23 多参数 operator[].
  decltype(auto) operator[](this auto& self, size_t x, size_t y) {
    return self.pixels_[self.index(x,y)];
  }

  T* data() noexcept {
    return pixels_.data();
  }

private:
  i32 w_ = 0;
  i32 h_ = 0;
  std::vector<T> pixels_;
};
  
} // namespace Medium