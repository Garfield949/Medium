///
/// @file   Material.h
/// @author Garfield949
/// @date   2026/2/14 11:42
/// @brief  
///
#pragma once

#include "Resources/Texture.h"

namespace Medium {

    class Material {
    public:
        Material() = default;

        std::string name; // 材质名称

        // 材质颜色与强度参数。
        // 前缀 "K" 代表「系数」，三个分量对应 R, G, B 三个通道的强度；
        // 1. Ambient,   环境光系数；
        // 2. Diffuse,   漫反射系数设为 0.8 是因为没有物质能 100% 将入射光漫反射出去；
        // 3. Specular,  镜面反射系数，决定高光强度；
        // 4. Emissive,  自发光系数。
        Vec3f Ka = {0.2f, 0.2f, 0.2f};
        Vec3f Kd = {1.0f, 1.0f, 1.0f};
        Vec3f Ks = {0.0f, 0.0f, 0.0f};
        Vec3f Ke = {0.0f, 0.0f, 0.0f};

        // 表面属性参数，控制光线与表面交互的细节。
        // 1. Shininess,           亮泽度，决定了高光的聚散（清晰度）程度；
        // 2. Index Of Refraction, 折射率；
        // 3. Dissolve,            即透明度 Alpha 的别称。
        f32 Ns = 0.0f;
        f32 Ni = 0.0f;
        f32 d  = 1.0f;

        // Illumination Model, 控制光照的计算公式。
        // 1 = 漫反射；2 = 漫反射 + 高光；
        int illum = 1;

        TexturePtr map_Ka   = nullptr; // 环境光贴图
        TexturePtr map_Kd   = nullptr; // 漫反射贴图
        TexturePtr map_Ks   = nullptr; // 镜面反射贴图
        TexturePtr map_d    = nullptr; // 透明度贴图
        TexturePtr map_bump = nullptr; // 法线（凹凸）贴图
    };

} // Medium