///
/// @file   Profiler.h
/// @author Garfield949
/// @date   2026/3/22 17:16
/// @brief  
///
#pragma once

#include <fmt/color.h>

#include "Base/Types.h"
#include "Pipeline/RenderStat.h"

namespace Medium {

class Profiler {
public:
  explicit Profiler() = default;
  
  // 设置信息报告间隔。
  void setReportInterval(f32 interval_s) noexcept { report_interval_s_ = interval_s; }
  
  void update(f32 dt) {
    report_acc_s_ += dt;
    frame_cnt_acc_++;
    if(report_acc_s_ >= report_interval_s_) {
      report();
      resetAccumulators();
    }
  }

  void collectStats(const RenderStat& stats) {
    draw_calls_acc_ += stats.draw_calls;
  }
  
private:
  // 重置累积时间和累积帧数
  void resetAccumulators() noexcept { 
    report_acc_s_   = 0.0f;
    frame_cnt_acc_  = 0;
    draw_calls_acc_ = 0;
  }

  void report() const noexcept {
    // 这里算的是规定报告时间间隔内的平均值
    f32 fps_avg_s          = static_cast<f32>(frame_cnt_acc_) / report_acc_s_;
    f32 frame_time_avg_ms  = (1.0f / fps_avg_s) * 1000.0f;
    u32 draw_calls_avg_fps = draw_calls_acc_ / frame_cnt_acc_;

    fmt::print("\rFPS: {:.3f} | Frame Time: {:.3f} ms | DrawCalls: {}      ",
      fps_avg_s, frame_time_avg_ms, draw_calls_avg_fps);
    std::fflush(stdout);
  }
  
  f32 report_interval_s_ = 1.0f; // 报告信息的时间间隔(s)

  f32 report_acc_s_      = 0.0f; // 时间累加器
  u32 frame_cnt_acc_     = 0.0f; // 帧数累加器
  u32 draw_calls_acc_    = 0;    // 绘制调用累加器
};

} // Medium