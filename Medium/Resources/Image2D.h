///
/// @file   Image2D.h
/// @author Garfield949
/// @date   2026/9/5 22:40
/// @brief  
///
#pragma once

#include <cassert>
#include <vector>

#include "Base/Types.h"

namespace Medium {

template<typename T>
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
  explicit Image2D(i32 w, i32 h): w_{w}, h_{h}, data_(w * h) {}
  
  i32 width()  const noexcept { return w_; }
  i32 height() const noexcept { return h_; }

  f32 widthF()  const noexcept { return static_cast<f32>(w_); }
  f32 heightF() const noexcept { return static_cast<f32>(h_); }

  size_t pixelCount() const noexcept { return data_.size(); }

  // Medium 在 读入模型 -> NDC -> 视口变换 链路上全程采用原点在左下角的坐标系。
  size_t index(i32 x, i32 y) const noexcept {
    assert(x >= 0 && x < w_ && y >= 0 && y < h_);
    return y * w_ + x;
  }

  void resize(i32 width, i32 height) {
    assert(width > 0 && height > 0);
    data_.resize(width * height);
    w_ = width;
    h_ = height;
  }

  void fill(const T& value) {
    std::fill(data_.begin(), data_.end(), value);
  }

  const T& operator[](size_t i) const {
    assert(i < data_.size());
    return data_[i];
  }

  const T& at(i32 x, i32 y) const {
    return data_[index(x, y)];
  }

  T& operator[](size_t i) {
    assert(i < data_.size());
    return data_[i];
  }

  T& at(i32 x, i32 y) {
    return data_[index(x, y)];
  }
  
private:
  i32 w_ = 0;
  i32 h_ = 0;
  std::vector<T> data_;
};
  
} // namespace Medium