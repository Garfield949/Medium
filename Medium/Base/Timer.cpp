///
/// @file Timer.cpp
/// @brief
///
#include "Timer.h"

#include <fmt/core.h>

namespace Medium {

  // 1. 两个 time_point 相加减得到一个 std::chrono::duration，其完整定义为
  //    std::chrono::duration<数据类型, 周期> ；
  // 2. 参数周期默认为 std::ratio<1>，即 1 秒；相比之下，std::ratio<1, 1000> 表示 1 毫秒。
  f32 Timer::tick() {
    // t2 - t1 得到的通常是纳秒的个数，这里会自动转换为秒数
    auto time_now = Clock::now();
    std::chrono::duration<f32> diff = time_now - time_last_tick_;

    delta_time_s_ = diff.count();
    time_last_tick_ = time_now;

    fps_report_accumulator_s_ += delta_time_s_;
    frame_cnt_accumulator_++;

    if(fps_report_accumulator_s_ >= fps_report_interval_s_) {
      // 规定时间间隔内的平均 FPS
      f32 fps_avg_s = static_cast<f32>(frame_cnt_accumulator_) / fps_report_accumulator_s_;
      f32 frametime_avg_ms = (1.0f / fps_avg_s) * 1000.0f;

      fmt::print("\rTimer::tick - [fps_avg]: {:.3f} fps | [frame_time_avg]: {:.3f} ms",
        fps_avg_s, frametime_avg_ms);
      std::fflush(stdout);

      resetAccumulators();
    }

    return delta_time_s_;
  }

} // namespace Medium
