///
/// @file   CSM.h
/// @author Garfield949
/// @date   2026/4/20 21:29
/// @brief  级联阴影映射 Cascaded Shadow Mapping
///
#pragma once
//   [ 透视走样 Perspective Aliasing ]
// - 平行光的阴影贴图是均匀、精度有限的；
// - 透视投影的近大远小现象导致，屏幕空间内近处的像素密度远大于远处，即同一大小的世界空间区域，在屏幕空间近处和远处
//   所占的像素数不同；
// - 近处：屏幕采样率 >> 阴影贴图采样率，导致很多个像素争抢一个 texel;
//   远处：屏幕采样率 <= 阴影贴图采样率，几乎能实现一对一的映射关系，精度很高。
// - 重点：并不是远处不会走样，而是远处的像素变稀疏，走样没有暴露出来。
namespace Medium {



} // namespace Medium
