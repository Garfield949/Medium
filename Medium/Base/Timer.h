///
/// @file Timer.h
/// @brief
///
#pragma once

#include <chrono>

#include "Types.h"

namespace Medium {

   /// @class Timer
   /// @brief 负责计算每一帧的 Delta Time 及报告统计信息。
  class Timer {
  public:
    explicit Timer() = default;

    // 每帧开始时调用。计算上一帧到现在的间隔、累积统计数据、报告信息并返回上一帧的 Delta Time(s)。
    f32 tick();

    // 设置信息报告间隔。
    void setReportInterval(f32 interval_s) noexcept { fps_report_interval_s_ = interval_s; }

    // 重置计时器
    void reset() noexcept { time_last_tick_ = Clock::now(); }

  private:
    // 重置累积时间和累积帧数
    void resetAccumulators() noexcept { fps_report_accumulator_s_ = 0.0f; frame_cnt_accumulator_ = 0; }

    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point time_last_tick_ = Clock::now(); // 记录上一次 tick 的时间点。
    f32 delta_time_s_                 = 0.0f;         // 记录上一帧的时间间隔
    f32 fps_report_interval_s_        = 1.0f;         // 报告 FPS 的时间间隔(s)
    f32 fps_report_accumulator_s_     = 0.0f;         // 累积时间
    u32 frame_cnt_accumulator_        = 0;            // 时间间隔内的累积帧数
  };

} // namespace Medium
