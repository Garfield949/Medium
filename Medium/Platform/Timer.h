///
/// @file Timer.h
/// @brief
///
#pragma once

#include <chrono>

#include "Base/Types.h"

namespace Medium {

/// @class Timer
/// @brief 负责计算每一帧的 Delta Time
class Timer {
public:
  explicit Timer() = default;

  // 每帧开始时调用。计算上一帧到现在的间隔、累积统计数据、报告信息并返回上一帧的 Delta Time(s)。
  f32 tick();

  // 重置计时器
  void reset() noexcept { time_last_tick_ = Clock::now(); }

private:
  using Clock = std::chrono::high_resolution_clock;
  Clock::time_point time_last_tick_ = Clock::now(); // 记录上一次 tick 的时间点。
  f32 delta_time_s_                 = 0.0f;         // 记录上一帧的时间间隔
};

} // namespace Medium
