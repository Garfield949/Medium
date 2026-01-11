#include <algorithm>

#include "Framebuffer.h"
#include "Math.h"

namespace Medium {

    Framebuffer::Framebuffer(unsigned width, unsigned height)
        :width_(width), height_(height),
         pixelBuf_(width * height, Color::Black().argb),
         depthBuf_(width * height, INF_F32){
    }

    void Framebuffer::clearPixel(const Color &color) {
        std::fill(pixelBuf_.begin(), pixelBuf_.end(), color.argb);
    }

    void Framebuffer::clearDepth() {
        std::fill(depthBuf_.begin(), depthBuf_.end(), INF_F32);
    }

    void Framebuffer::setPixel(u32 x, u32 y, const Color &color) {
        pixelBuf_[index(x, y)] = color.argb;
    }
}// namespace Medium
