///
/// @file   Texture2D.h
/// @author Garfield949
/// @date   2026/6/11 14:24
/// @brief  
///
#pragma once

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cstddef>

#include <fmt/base.h>
#include <fmt/color.h>

#include "Base/Log.h"
#include "Base/Types.h"
#include "Base/Color.h"

namespace Medium {

enum class PixelFormat {
  R32F,
  ABGR8,  
  RGBA32F
};

class Texture2D {
public:
  // 禁用拷贝构造和拷贝赋值，防止无意识复制行为；
  // 保留移动构造和移动赋值：
  // - Texture2D image = LoadImage(path);  <-- 返回右值；
  // - Texture2D image2{std::move(image)};
  Texture2D(const Texture2D&) = delete;
  Texture2D& operator=(const Texture2D&) = delete;
  // 编译器的奇怪特性：若显式定义了
  // - 拷贝构造；
  // - 移动构造；
  // - 拷贝赋值；
  // - 移动赋值；
  // - 析构函数;
  // 这 5 个中的至少 1 个，那么其它 4 个不会被隐式生成，必须手动定义，因为编译器认为我主动介入了该类型的赋值行为。
  Texture2D(Texture2D&&) noexcept = default;
  Texture2D& operator=(Texture2D&&) noexcept = default;

  // 调用方必须显式手动进行 resize 来分配空间，并进行错误处理。
  // 当 Texture2D 作为 U32 Packed 颜色缓冲时，从中读出一个 LinearColor 需用到 ColorSpace 参数进行解码。
  explicit Texture2D(PixelFormat format, ColorSpace space)
    : pixel_format_{format}, color_space_{space} {}

  // 接管传入的裸指针。用在 AssetCache 读入 stbi 裸 unsigned char*.
  explicit Texture2D(u32 width, u32 height, PixelFormat format, ColorSpace space, void* data) 
  : Texture2D{format, space} {
    assert(data);
    w_ = width;
    h_ = height;
    data_.reset(static_cast<std::byte*>(data));
  }

  static size_t BytesPerPixel(PixelFormat format) noexcept {
    switch(format) {
    case PixelFormat::R32F:                // 深度纹理格式；
    case PixelFormat::ABGR8:   return 4;   // stbi 读入纹理格式 & SDL 屏幕缓冲交付格式：低地址 -> RR GG BB AA -> 高地址；
    case PixelFormat::RGBA32F: return 16;  // LinearColor HDR 帧缓冲格式；
    }
  }

  u32 width()   const noexcept { return w_; }
  u32 height()  const noexcept { return h_; }
  f32 widthF()  const noexcept { return static_cast<f32>(w_); }
  f32 heightF() const noexcept { return static_cast<f32>(h_); }

  size_t pixelCount() const noexcept { return static_cast<size_t>(w_) * static_cast<size_t>(h_); }

  // Medium 在 读入模型 -> NDC -> 视口变换 链路上全程采用原点在左下角的坐标系。
  size_t index(i32 x, i32 y) const noexcept {
    assert(x >= 0 && x < w_ && y >= 0 && y < h_);
    return y * w_ + x;
  }

  // ---------- Packed Argb8

  // 用途：读取 present 颜色缓冲。
  LinearColor readPackedAbgrToLinear(size_t index) const noexcept {
    assert(this->pixel_format_ == PixelFormat::ABGR8);
    Color abgr = reinterpret_cast<Color*>(data_.get())[index];
    return abgr.toLinear(this->color_space_);
  }

  // 用途：读取 stbi 贴图原始颜色。
  LinearColor readPackedAbgrToLinear(i32 x, i32 y) const noexcept {
    assert(this->pixel_format_ == PixelFormat::ABGR8);
    return readPackedAbgrToLinear(index(x, y));
  }

  /// @brief 颜色写入，U32 Color 专用。
  /// @details index 必须由 screenIndex() 或 index() 计算得到。
  void writePackedAbgr(size_t index, u32 packed_abgr8) {
    assert(this->pixel_format_ == PixelFormat::ABGR8);
    reinterpret_cast<u32*>(data_.get())[index] = packed_abgr8;
  }

  // ---------- LinearColor

  LinearColor readLinearColor(size_t index) const noexcept {
    assert(this->pixel_format_ == PixelFormat::RGBA32F);
    return reinterpret_cast<LinearColor*>(data_.get())[index];
  }

  /// @brief 颜色写入，HDR LinearColor 专用。
  /// @details index 必须由 screenIndex() 或 index() 计算得到。
  void writeLinearColor(size_t index, const LinearColor& color) {
    assert(this->pixel_format_ == PixelFormat::RGBA32F);
    reinterpret_cast<LinearColor*>(data_.get())[index] = color;
  }

  // ---------- Depth

  // 用于深度缓冲取值 & Shadow Map 采样。
  // 注：Shadow Pass 和 Main Pass走的一个管线，rasterizer 里都要通过 getIndex() 进行索引，而这个索引是针对屏幕
  // 坐标进行了 flipped y 翻转的，所以采样时也要通过 getIndex() 取值。
  f32 readDepth(size_t index) const noexcept {
    assert(this->pixel_format_ == PixelFormat::R32F);
    return reinterpret_cast<f32*>(data_.get())[index];
  }

  /// @brief 深度写入，32 位浮点专用。
  /// @details index 必须由 screenIndex() 或 index() 计算得到。
  void writeDepth(size_t index, f32 value) {
    assert(this->pixel_format_ == PixelFormat::R32F);
    reinterpret_cast<f32*>(data_.get())[index] = value;
  }

  // LinearColor HDR 帧缓冲专用
  void fill(const LinearColor& color) {
    assert(this->pixel_format_ == PixelFormat::RGBA32F);
    std::fill_n(reinterpret_cast<LinearColor*>(data_.get()), pixelCount(), color);
  }

  // 深度缓冲专用
  void fill(f32 value) {
    assert(this->pixel_format_ == PixelFormat::R32F);
    size_t pixels = pixelCount();
    if(value == 0.0f)
      // 注意 memset 是按 byte 填充的，即欲填充值的位模式不能超过 8. 参数类型是 int 但在内部会被强制截断为一
      // 个 unsigned char. 比如：
      // int arr[10];
      // std::memset(arr, 1, sizeof(arr));
      // 这是错的。填充完后，arr 指向内存的位模式是这样的：0x...010101... 然而访问 arr[] 时会按照 32 位读到
      // 一个 0x01010101，十进制的 16843009 而不是 1.
      std::memset(data_.get(), 0, pixels * sizeof(f32));
    else
      // 若填充值为单字节的 memset 最快，否则改用遍历式写入。
      std::fill_n(reinterpret_cast<f32*>(data_.get()), pixels, value);
  }

  u32* packedAbgr8Data() const {
    assert(this->pixel_format_ == PixelFormat::ABGR8);
    return reinterpret_cast<u32*>(data_.get());
  }

  [[nodiscard]]
  bool resize(u32 new_width, u32 new_height) {
    if(new_width == w_ && new_height == h_) return true;

    if(new_width == 0 || new_height == 0) {
      Log::printLogPrefix("Texture2D", "resize", true);
      fmt::print("Invalid size [{} x {}]\n", new_width, new_height);
      return false;
    }

    size_t w = static_cast<size_t>(new_width);
    size_t h = static_cast<size_t>(new_height);
    size_t bpp = BytesPerPixel(this->pixel_format_);
    size_t max_size = std::numeric_limits<size_t>::max();

    // if w * h > max_size. 写成除法是为了防止溢出。
    if(w > max_size / h) {
      Log::printLogPrefix("Texture2D", "resize", true);
      fmt::print("Pixel count overflow [{} x {}]\n", new_width, new_height);
      return false;
    }

    size_t pixels = w * h;

    // if pixels * bpp > max_size.
    if(pixels > max_size / bpp) {
      Log::printLogPrefix("Texture2D", "resize", true);
      fmt::print("Byte size overflow [{} x {}]\n", new_width, new_height);
      return false;
    }

    size_t new_size = pixels * bpp;

    auto old_ptr = data_.release();
    // 若 realloc 目标大小为 0，则行为变成释放 old_ptr 指向的内存块并返回 nullptr，是一次成功操作；
    // 若 realloc 选择在原地进行扩大 / 缩小，那么分配成功后 old 和 new 将指向同一块内存地址；
    // 若 realloc 选择在别处开辟新空间，会自动将 old 内存块拷贝到新内存块，并 free 掉旧内存。
    auto new_ptr = static_cast<std::byte*>(std::realloc(old_ptr, new_size));
    if(!new_ptr) {
      Log::printLogPrefix("Texture2D", "resize", true);
      fmt::print("Reallocation failed for [{} x {}]\n", new_width, new_height);
      data_.reset(old_ptr);
      return false;
    }

    w_ = new_width;
    h_ = new_height;
    // reset 的行为是先用 TextureDeleter 释放掉持有的内存块，再置为新内存地址。
    data_.reset(new_ptr);

    return true;
  }

private:
  u32 w_ = 0;
  u32 h_ = 0;

  PixelFormat pixel_format_;
  ColorSpace color_space_; // 该参数目前仅当 Texture2D 作为 stbi 贴图时有用。

  // 自定义删除器，否则 unique_ptr 默认析构(delete)会产生未定义行为。
  // 分配器和删除器必须严格对应，混用会导致 UB: 
  // - new    -> delete;
  // - new[]  -> delete[];
  // - malloc -> free.
  struct SafeDeleter {
    void operator()(std::byte* data) const { std::free(data); }
  };
  // 使用 C 风格内存分配，以兼容 stbi.
  // C++17 引入 std::byte，在语义上区分「占了一个字节的数值」和「内存字节」, std::byte 禁用算术运算，只支持位运算；
  std::unique_ptr<std::byte, SafeDeleter> data_ = nullptr;
};

} // namespace Medium
