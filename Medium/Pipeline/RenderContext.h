///
/// @file   RenderContext.h
/// @author Garfield949
/// @date   2026/2/13 16:45
/// @brief  
///
#pragma once

namespace Medium {

    class  Rasterizer;
    class  RenderTarget;
    struct IShader;
    class  Camera;

    struct RenderContext {
        RenderContext(Rasterizer &r, RenderTarget &t, IShader &s, Camera &c)
            : rasterizer(r), render_target(t), shader(s), camera(c) {}
        Rasterizer   &rasterizer;
        RenderTarget &render_target;
        IShader      &shader;
        Camera       &camera;
    };

} // namespace Medium
