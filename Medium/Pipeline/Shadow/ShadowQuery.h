///
/// @file   ShadowQuery.h
/// @author Garfield949
/// @date   2026/4/16 22:52
/// @brief
#pragma once

#include "Base/Types.h"
#include "Base/Math/Math.h"
#include "Pipeline/Shadow/CSM.h"
#include "Pipeline/FrameBuffer.h"

namespace Medium {
//                                     [ 自影伪影问题 Shadow Acne ]
// - Shadow Map 的分辨率不足，场景中任意一个表面区域附近可能会有若干个点映射到阴影纹理的同一个深度值上，导致在主 Pass 查询
//   深度时，当前点的实际深度大于在纹理中记录的值，造成“自己遮挡自己”的视觉错误；
// - 光线方向与表面越倾斜，误差越大：想象一下，此时光源视野里的一块正方形区域投射至斜面上时，会覆盖更长的物理距离，使得单位区
//   域里有更多点被映射到同一个深度上；
// - 简单的深度偏移法 Slope-Scaled Depth Bias：计算一个偏置值 bias，让着色点往光源方向更靠近点，手动让产生误差的计算躲开
//   临界点。比如将 bias 控制在 [0.001, 0.005] 之间，光线与表面的夹角越大，则 bias 越大，符合语义。实现如下：
//          f32 bias = std::max(0.005f * (1.0f - n_dot_l), 0.001f); if(z_cur + bias < z_map);
// - 注意彼得潘现象：加上 bias 的本质是将光源视野里的最近深度往更近处推 (reversed-z 语境下，Zndc 越靠近 1 则离原点越近)，
//   导致本该在阴影中的着色点，即靠近 shadow caster 根部的地面在进行深度比较时会被错判为不在阴影中而受光，有一种悬浮的现象。

//                                     [ 法线偏置 Normal Bias ]
// - 特点：临时将 frag 的世界物理坐标沿几何法向外推一定距离 --- 即进行 Shadow Mapping 时，用到的是模型的 “悬浮外壳”；
// - 主要解决自影伪影：
//   1. Depth Bias 的缺陷在于，其仅是在光空间内将 frag 往高处提；表面很倾斜的时候需加一个很大的 Bias 才能超过当前 frag
//       所属 texel 覆盖区域量化的那个深度；
//   2. Normal Bias 的优势在于，其几何的位移不依赖光空间，光源视角下的效果为：
//      1) 与光线几乎垂直的地面在世界空间法向的凸起恰好就是在光空间内的 z 轴位移，此时的 aliasing 程度最轻，只需一点轻微的
//         bias 补偿即可跃出量化值；
//      2) 对于倾斜表面，想象一个斜坡的横截面 --- 左高右低，则坡面的垂直法向指向右上方；光线垂直从高处往下发射。故在该法向上
//         位移会产生两个效果：
//         a. 当前 frag 在光空间内的 z 值会稍微变小一些（离光更近），天然有利于跃出当前 texel 的量化值，对应 depth_frag
//            变大（翻转 z 语境）；
//         b. 在光空间的 xy 轴产生较大位移，必然会使 uv 采样往被量化得更深的一格 texel 跳跃（法向的水平分量必然指向更靠近
//            坡底的位置），从而使 uv 漂移，即 depth_from_map 变小（翻转 z 语境）。和效果 a 加起来进一步使 depth_frag
//            明显大于 depth_from_map，从而高效率地解决自阴影。
// - 次要影响：改善 Peter Panning: 由前述可知道，简单的 Depth Bias 是直接在 z 值上应用一个巨大的 bias，不可避免地导致彼
//   得潘现象；相比单纯堆叠 Depth Bias，normal bias 往往能以更小的偏置解决自阴影，因此通常更不容易产生严重的 Peter Panning.
// - 补充：其实 Depth Bias 也是一种几何修改技术，只不过仅修改 frag 在光空间内的 z 使其抬升。
inline Vec3f calcNormalBias(const Vec3f& frag_n,
                            const f32    nl,
                            const f32    texel_size_ws)
{
  // 点积设置一个安全值，防止除零
  f32 safe_nl = std::max(nl, Math::KindaSmallNumber);
  // - Shadow map 的一个纹素投影到世界空间物体表面时，会有深度 aliasing 现象：
  //   1) 若光线与表面垂直，则 aliasing 程度最轻；
  //   2) 光线越呈掠射角（越与平面平行），一个纹素投影到表面的面积会拉长，加深 aliasing (深度跨度很大的多个点被量化到同一个
  //      深度值上).
  // - slope 即 tanθ:
  //   1) 有 nl = cosθ，所以 tanθ = sinθ / cosθ = sqrt(1 - nl^2) / nl;
  //   2) tanθ 的性质：光线与表面垂直时，其值为 0; 光线与表面趋于平行时，slope 趋于无穷（设置 safe_nl 的原因），该性质符
  //      合 Normal Bias 的要求 --- 即 aliasing 程度越高，当前 frag 的法向几何位移应越大。
  f32 sin2 = std::max(0.0f, 1 - safe_nl * safe_nl);
  // slope 上限不要给太大：几乎与光平行的面会造成极小的 safe_nl 从而直接取到斜率上限，然而在光空间里如此大的 normal bias,
  // 沿墙法向偏移时对 light space z 几乎没有改善，反而在 light space xy 上有巨大偏移，造成阴影比较错误；
  // 测试过将 slope 计算封装出去，从而只计算一次（sampleVisibility会调用两次），但无收益。
  f32 slope = std::min(std::sqrt(sin2) / safe_nl, 1.0f);
  // - 法向位移距离。纹素大小起到自适应调节：Shadow Map 分辨率越小，每 texel 覆盖世界范围越大，aliasing 会加重，bias 会
  //   自动调整变大。
  f32 bias = slope * texel_size_ws;

  return bias * frag_n;
}

inline f32 sampleVisibility(const CSMContext* csm_ctx,
                            const CSMCascade* cascade,
                            const f32    nl,
                            const Vec3f& frag_pos,
                            const Vec3f& frag_n) {
  f32 visibility = 1.0f;
  
  Vec3f frag_pos_biased = frag_pos + calcNormalBias(frag_n, nl, cascade->texel_size_ws);

  // 这个变量其实叫 frag_clip_ls. 但正交投影 w = 1，直接跳过透视除法得到 ndc.
  Vec4f frag_ndc_ls = cascade->mat_vp_light * frag_pos_biased.homogeneous();

  Vec2f shadow_uv{
    (frag_ndc_ls.x() + 1.0f) * 0.5f,
    (frag_ndc_ls.y() + 1.0f) * 0.5f
  };

  if(Math::isBetween(frag_ndc_ls.z(), 0.0f, 1.0f) &&
     Math::isBetween(shadow_uv.x()  , 0.0f, 1.0f) &&
     Math::isBetween(shadow_uv.y()  , 0.0f, 1.0f))
  {
    f32 z_frag = frag_ndc_ls.z();

    f32 z_map = csm_ctx->csm_atlas->sampleDepth(shadow_uv.x(), shadow_uv.y(), cascade->viewport_atlas);

    constexpr f32 z_bias = 0.0001f;

    if(z_frag + z_bias < z_map) visibility = 0.2f;
  }

  return visibility;
}

inline f32 queryCSMVisibility(const CSMContext* csm_ctx,
                              const f32    nl,
                              const Vec3f& frag_pos,
                              const Vec3f& frag_n,
                              const f32    frag_z_view) {
  f32 visibility = 1.0f;

  auto cascade_query = csm_ctx->cascadeQuery(frag_z_view);

  if(cascade_query) {
    auto [cascade, index] = cascade_query.value();

    visibility = sampleVisibility(csm_ctx, cascade, nl, frag_pos, frag_n);

    f32 band_width = csm_ctx->blendBandWidth(index);

    f32 blend_start = cascade->slice_far - band_width;

    f32 blend_end = cascade->slice_far;

    // 非尾级联且在混合带里。
    if(index + 1 < csm_ctx->cascade_cnt && frag_z_view >= blend_start && frag_z_view <= blend_end) {
      f32 blend_offset = frag_z_view - blend_start;
      
      f32 t = blend_offset / band_width;

      const CSMCascade* cascade_next = &csm_ctx->cascades[index + 1];

      f32 vsb_next = sampleVisibility(csm_ctx, cascade_next, nl, frag_pos, frag_n);

      visibility = Math::lerp(visibility, vsb_next, t);
    }
  }
  
  return visibility;
}
  
} // namespace Medium
