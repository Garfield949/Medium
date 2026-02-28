///
/// @file Shaders.h
/// @brief 着色器
///
#pragma once

#include "Base/Color.h"
#include "Base/Types.h"
#include "Pipeline/Shader/IShader.h"
#include "Base/Math.h"
#include "Resources/Texture.h"

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

        bool fragment(const Vec3f& weights, Color& color_final) override {
            Vec3f w_invs{varyings[0].w_inv, varyings[1].w_inv, varyings[2].w_inv};

            Vec3f n_interp = Math::interpolate(varyings[0].normal,
                                               varyings[1].normal,
                                               varyings[2].normal, weights, w_invs);

            n_interp.normalize();

            Vec2f uv_interp = Math::interpolate(varyings[0].uv,
                                                varyings[1].uv,
                                                varyings[2].uv, weights, w_invs);

            Vec2f UV{uv_interp.x(), uv_interp.y()};
            
            auto sampleColor = [&](const Texture* tex, const ColorSource src, const Vec2f& uv) -> Color {
                switch(src) {
                    case ColorSource::Checkerboard:
                        return Texture::getCheckerBoardColor(uv);
                    case ColorSource::Textured:
                        return tex->sample(uv);
                    case ColorSource::SolidColor:
                        return {uniforms.mat_params.Kd * 255.0f};
                }
            };

            Color diffuse_color = sampleColor(uniforms.map_Kd, uniforms.color_source_diffuse, UV);

            // 漫反射 Lambert 与观察方向无关，即从任意角度观察着色点，漫反射亮度均一致。
            f32 diff = std::max(n_interp.dot(uniforms.light_dir), 0.0f);
            // Half-Lambert
            diff = std::powf(diff * 0.5f + 0.5f, 2);

            bool enable_specular  = (uniforms.mat_params.illum >= 2) && 
                                    (uniforms.map_Ks != nullptr || !uniforms.mat_params.Ks.isZero(Math::KINDA_SMALL_NUMBER));

            Color specular_color;
            if(enable_specular) {
                Vec3f pos_world_interp = Math::interpolate(varyings[0].pos_world,
                                                           varyings[1].pos_world,
                                                           varyings[2].pos_world, weights, w_invs);

                Vec3f view_dir = (uniforms.camera_pos - pos_world_interp).normalized();

                Vec3f half_way = (view_dir + uniforms.light_dir).normalized();

                f32 spec = std::pow(std::max(n_interp.dot(half_way), 0.0f), uniforms.mat_params.Ns);

                specular_color = sampleColor(uniforms.map_Ks, uniforms.color_source_specular, UV) * spec;
            }

            color_final = diffuse_color * diff + specular_color;

            return false;
        }

    };

} // namespace Medium
