///
/// @file Shaders.h
/// @brief 着色器
///
#pragma once

#include "Pipeline/Shader/IShader.h"
#include "Core/Math/Math.h"

namespace Medium {

    struct SimpleShader : IShader {

        Vec4f vertex(const u32 nth_vert, const Vertex& vert) override {
            Vec4f pos_clip  = uniforms.wvp * vert.pos.homogeneous();
            Mat3f world_rot = uniforms.world.topLeftCorner(3, 3);

            // 维护当前顶点的 Varyings. 由于顶点法线只有方向，故变换到世界空间只需应用旋转。
            varyings[nth_vert].uv        = vert.tex_coord;
            varyings[nth_vert].normal    = (world_rot * vert.normal).normalized();
            varyings[nth_vert].pos_world = (uniforms.world * vert.pos.homogeneous()).head(3);
            varyings[nth_vert].w_inv     = 1.0f / pos_clip.w();

            return pos_clip;
        }

        bool fragment(const Vec3f& weights, Color& color) override {
            Vec3f w_invs{varyings[0].w_inv, varyings[1].w_inv, varyings[2].w_inv};

            Vec3f n_interp = Math::interpolate(varyings[0].normal,
                                                varyings[1].normal,
                                               varyings[2].normal, weights, w_invs);

            Vec2f uv_interp = Math::interpolate(varyings[0].uv,
                                                varyings[1].uv,
                                                varyings[2].uv, weights, w_invs);

            Vec2f UV{uv_interp.x(), uv_interp.y()};

            Color sample_color;
            if(uniforms.draw_checkerboard) {
                sample_color = Texture::getCheckerBoardColor(UV);
            } else if(texture != nullptr) {
                sample_color = texture->sample(UV);
            } else {
                sample_color = {
                    static_cast<u8>(uniforms.Kd.x() * 255),
                    static_cast<u8>(uniforms.Kd.y() * 255),
                    static_cast<u8>(uniforms.Kd.z() * 255)
                };
            }

            // 简单兰伯特光照
            f32 intensity = std::max(0.2f, n_interp.dot(uniforms.light_dir.normalized()));

            color = sample_color * intensity;

            return false;
        }

    };

} // namespace Medium
