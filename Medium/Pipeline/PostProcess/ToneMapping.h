///
/// @file   ToneMapping.h
/// @author Garfield949
/// @date   2026/7/6 13:41
/// @brief  
///
#pragma once

namespace Medium {
  class FrameBuffer;
}

namespace Medium::PostProcess {

void toneMapping(FrameBuffer& frame_buffer) noexcept;

} // namespace Medium::PostProcess
