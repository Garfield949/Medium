///
/// @file Shaders.h
/// @brief 着色器
///
#pragma once

#include "Base/Color.h"
#include "Base/Types.h"
#include "Base/Math.h"
#include "Base/Primitives/Vertex.h"
#include "Resources/Material.h"
#include "Resources/Texture.h"

namespace Medium {

    enum class ColorSource {
        Checkerboard, Textured, SolidColor
    };

    // Varying 是单个 draw call 里不断被插值变化的量，描述了属性如何从顶点的“点数据”
    // 演变成覆盖整个三角形面的“面数据”。
    struct Varying {
        Vec2f uv;        // 纹理坐标
        Vec3f normal;    // 顶点变到世界空间里的法向
        Vec3f pos_world; // 某些计算（比如光照）依赖顶点世界坐标
        f32   w_inv;     // 保存顶点的 1/w
    };

    // 单个 draw call 中不变的量
    struct Uniform {
        Mat4f world;
        Mat4f wvp;
        Vec3f camera_pos; // 用于高光着色
        Vec3f light_dir;

        MaterialParams mat_params;

        const Texture* map_Kd = nullptr;
        const Texture* map_Ks = nullptr;

        ColorSource color_source_diffuse  = ColorSource::Checkerboard;
        ColorSource color_source_specular = ColorSource::Checkerboard;
    };

    struct BlinnPhongShader {
        std::array<Varying, 3> varyings;
        Uniform uniforms;

        Vec4f vertex(const u32 nth_vert, const Vertex& vert) {
            Vec4f pos_clip  = uniforms.wvp * vert.pos.homogeneous();
            Mat3f world_rot = uniforms.world.topLeftCorner(3, 3);

            // 维护当前顶点的 Varyings. 由于顶点法线只有方向，故变换到世界空间只需应用旋转。
            varyings[nth_vert].uv        = vert.tex_coord;
            varyings[nth_vert].normal    = (world_rot * vert.normal).normalized();
            varyings[nth_vert].pos_world = (uniforms.world * vert.pos.homogeneous()).head(3);
            varyings[nth_vert].w_inv     = 1.0f / pos_clip.w();

            return pos_clip;
        }

        bool fragment(const Vec3f& weights, LinearColor& color_final) {
            Vec3f w_invs{varyings[0].w_inv, varyings[1].w_inv, varyings[2].w_inv};

            Vec3f n_interp = Math::interpolate(varyings[0].normal,
                                               varyings[1].normal,
                                               varyings[2].normal, weights, w_invs);

            n_interp.normalize();

            Vec2f uv_interp = Math::interpolate(varyings[0].uv,
                                                varyings[1].uv,
                                                varyings[2].uv, weights, w_invs);

            Vec2f UV{uv_interp.x(), uv_interp.y()};
            
            auto sampleColor = [&](const Texture* tex, const ColorSource src, const Vec2f& uv) -> LinearColor {
                switch(src) {
                    case ColorSource::Textured: 
                        return tex->sample(uv).toLinear(ColorSpace::SRGB);
                    case ColorSource::SolidColor:
                        return uniforms.mat_params.Kd;
                    default:
                        return Texture::getCheckerBoardColor(uv);
                }
            };

            LinearColor diffuse_color = sampleColor(uniforms.map_Kd, uniforms.color_source_diffuse, UV);

            // 漫反射 Lambert 与观察方向无关，即从任意角度观察着色点，漫反射亮度均一致。
            f32 diff = std::max(n_interp.dot(uniforms.light_dir), 0.0f);
            // Half-Lambert
            f32 half_lambert = diff * 0.5f + 0.5f;
            diff = half_lambert * half_lambert;

            bool enable_specular  = (uniforms.mat_params.illum >= 2) && 
                                    (uniforms.map_Ks != nullptr || !uniforms.mat_params.Ks.isBlack(Math::VERY_SMALL_NUMBER));

            LinearColor specular_color;
            if(enable_specular) {
                Vec3f pos_world_interp = Math::interpolate(varyings[0].pos_world,
                                                           varyings[1].pos_world,
                                                           varyings[2].pos_world, weights, w_invs);

                Vec3f view_dir = (uniforms.camera_pos - pos_world_interp).normalized();

                Vec3f half_way = (view_dir + uniforms.light_dir).normalized();

                f32 spec = std::powf(std::max(n_interp.dot(half_way), 0.0f), uniforms.mat_params.Ns);

                specular_color = sampleColor(uniforms.map_Ks, uniforms.color_source_specular, UV) * spec;
            }

            color_final = diffuse_color * diff + specular_color;

            return false;
        }

    };

} // namespace Medium
