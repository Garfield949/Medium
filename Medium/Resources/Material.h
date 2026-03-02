///
/// @file   Material.h
/// @author Garfield949
/// @date   2026/2/14 11:42
/// @brief  
///
#pragma once

#include "Base/Color.h"
#include "Base/Types.h"
#include "Base/Forwards.h"

namespace Medium {

    struct MaterialParams {
        // 材质颜色与强度参数。
        // 前缀 "K" 代表「系数」，三个分量对应 R, G, B 三个通道的强度；
        // 1. Ambient,   环境光系数；
        // 2. Diffuse,   漫反射系数设为 0.8 是因为没有物质能 100% 将入射光漫反射出去；
        // 3. Specular,  镜面反射系数，决定高光强度；
        LinearColor Ka = {0.2f, 0.2f, 0.2f};
        LinearColor Kd = {1.0f, 1.0f, 1.0f};
        LinearColor Ks = {0.0f, 0.0f, 0.0f};
        LinearColor Ke = {0.0f, 0.0f, 0.0f};
        // 表面属性参数，控制光线与表面交互的细节。
        // 1. Shininess,           亮泽度，决定了高光的聚散（清晰度）程度；
        // 2. Index Of Refraction, 折射率；
        // 3. Dissolve,            不透明度，1.0 表示完全不透明。
        // 4. Transmission filter, 滤光颜色，描述光线穿过材质时什么颜色不会被吸收。
        f32 Ns = 0.0f;
        f32 Ni = 0.0f;
        f32 d  = 1.0f;
        f32 Tf = 1.0f;
        // Illumination Model, 控制光照的计算公式。
        // 0 = 无光照，直接输出 Kd；
        // 1 = 漫反射；
        // 2 = 漫反射 + 高光；
        int illum = 2;
    };

    class Material {
    public:
        Material() = default;

        std::string name; // 材质名称

        MaterialParams params;

        TexturePtr map_Ka   = nullptr; // 环境光贴图
        TexturePtr map_Kd   = nullptr; // 漫反射贴图
        TexturePtr map_Ks   = nullptr; // 镜面反射贴图
        TexturePtr map_d    = nullptr; // 透明度贴图
        TexturePtr map_bump = nullptr; // 法线（凹凸）贴图
    };

} // Medium