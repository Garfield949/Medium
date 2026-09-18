///
/// @file   BlinnShader.cpp
/// @author Garfield949
/// @date   2026/3/6 09:09
/// @brief  
///
#include "BlinnShader.h"

#include <array>

#include "Pipeline/Shadow/CSM.h"
#include "Resources/Vertex.h"
#include "Resources/Material.h"
#include "Resources/Sampler.h"
#include "Pipeline/RenderContext.h"
#include "Pipeline/Shadow/ShadowQuery.h"

namespace Medium {
  
void BlinnShader::bind(const ShaderBindContext& bind_ctx, const Mat4f& world, const Material* mat) noexcept {
  const CameraContext* cam_ctx = bind_ctx.camera_ctx;
  const LightingContext* lighting_ctx = bind_ctx.lighting_ctx;

  this->uniforms = {
    .camera_pos    = cam_ctx->cam_pos,
    .mat_wvp       = cam_ctx->mat_vp * world,
    .mat_world     = world,
    .material      = mat,

    .light_dir_inv = -lighting_ctx->light_dir,
    .light_color   = lighting_ctx->light_color,
    
    .csm_ctx = lighting_ctx->csm_ctx,
  };
}

VSOut BlinnShader::vertex(const Vertex& vert) const noexcept {
  // 只表示方向的向量变换到世界空间只需应用旋转。
  Mat3f rot_world = uniforms.mat_world.topLeftCorner(3, 3);  
  
  Vec3f vert_n_world = (rot_world * vert.normal).normalized();
  Vec3f vert_t_world =  rot_world * vert.tangent.head<3>();
  Vec3f vert_p_world = (uniforms.mat_world * vert.pos.homogeneous()).head<3>();

  VSOut vs_out;
  vs_out.pos_clip      = uniforms.mat_wvp * vert.pos.homogeneous();
  vs_out.varying.w_inv = 1.0f / vs_out.pos_clip.w();
  // tangent 的 w 分量（手性）也一并 smooth 插值，测试过值也只会是 1 或 -1 没有问题；
  // 测试过手动一个一个填 scalar，性能无差异。
  vs_out.varying.store(vert.uv.data()     , kOffsetUV     , 2);
  vs_out.varying.store(vert_n_world.data(), kOffsetNormal , 3);
  vs_out.varying.store(vert_t_world.data(), kOffsetTangent, 3);
  vs_out.varying.store(&vert.tangent.w()  , kOffsetDet    , 1);
  vs_out.varying.store(vert_p_world.data(), kOffsetPos    , 3);
  return vs_out;
}

bool BlinnShader::fragment(const FSIn& fs_in, LinearColor& frag_color) const noexcept
{
  // frag_n 为几何法线 Geometry Normal.
  Vec2f frag_uv  = fs_in.getVec2f(kOffsetUV);
  Vec3f frag_n   = fs_in.getVec3f(kOffsetNormal).normalized();
  Vec3f frag_pos = fs_in.getVec3f(kOffsetPos);

  const Material* mat = uniforms.material;

  LinearColor diffuse = sampleSlot(mat->diff_slot, frag_uv);

  f32 frag_alpha = 1.0f;
  if(mat->surface_mode == Material::SurfaceMode::Masked) { // Alpha Test
    f32 value = diffuse.A;
    if(value < Material::kAlphaCutoff) return true;
  } else if(mat->surface_mode == Material::SurfaceMode::Transparent) {
    frag_alpha = mat->params.d;
  }

  const auto csm_ctx = uniforms.csm_ctx;

  f32 frag_z_view = 0.0f;
  if(csm_ctx) frag_z_view = (csm_ctx->mat_view_snapshot * frag_pos.homogeneous()).z();

  // Cascade Band View.
  // csm_ctx 理论上只会在 ImGui 设置 Draw Mode 为 Wireframe 或者方向光取消 Cast Shadows 时，才会被 RenderManager
  // 传一个 nullptr 表示禁用 Shadow Query.
  if(csm_ctx && csm_ctx->cascade_band_view_enabled) {
    auto cascade_query = csm_ctx->cascadeQuery(frag_z_view);

    // 不在级联范围内的 frag 会进入下方的正常着色逻辑。
    if(cascade_query) {
      auto [cascade, index] = cascade_query.value();

      Vec4f frag_ndc_ls = cascade->mat_vp_light * frag_pos.homogeneous();

      Vec2f shadow_uv{
        (frag_ndc_ls.x() + 1.0f) * 0.5f,
        (frag_ndc_ls.y() + 1.0f) * 0.5f
      };

      if(Math::isBetween(frag_ndc_ls.z(), 0.0f, 1.0f) &&
         Math::isBetween(shadow_uv.x()  , 0.0f, 1.0f) &&
         Math::isBetween(shadow_uv.y()  , 0.0f, 1.0f))
      {
        static constexpr std::array colors = {
          LinearColor::Red(), LinearColor::Green(), LinearColor::Blue(), LinearColor::Pink()
        };

        frag_color = colors[index];

        f32 band_width  = csm_ctx->blendBandWidth(index);
        f32 blend_start = cascade->slice_far - band_width;
        f32 blend_end   = cascade->slice_far;

        // 非尾级联且在混合带里。
        if(index + 1 < csm_ctx->cascade_cnt && frag_z_view >= blend_start && frag_z_view <= blend_end) {
          f32 blend_offset = frag_z_view - blend_start;
          f32 t = blend_offset / band_width;

          frag_color = Math::lerp(colors[index], colors[index + 1], t);
        }

        frag_color.A = frag_alpha;

        return false;
      }
    }
  }

  // N dot L, 无 Normal Mapping 扰动。
  const f32 nl = frag_n.dot(uniforms.light_dir_inv);

  // 该值为 1.0 代表当前 frag 不在阴影当中。
  f32 frag_visibility = 1.0f;
  if(csm_ctx) {
    if(nl <= 0.0f) {
      frag_visibility = 0.3f; // 背对光源则直接判定处于阴影中。
    } else {
      frag_visibility = queryCSMVisibility(csm_ctx, nl, frag_pos, frag_n, frag_z_view);
    }
  }

  if(mat->enable_norm) {
    Vec3f T = fs_in.getVec3f(kOffsetTangent);
    f32 det = fs_in.getScalar(kOffsetDet);

    // 施密特正交化
    T = (T - T.dot(frag_n) * frag_n).normalized();

    // T 和 N 均已在顶点着色器中应用了 world 矩阵，B 自然也附带了 world 的变换效果
    // 同一面片上顶点的 det 一定相同，所以插值出来的顶点的 det 可以直接取 varyings 其中之一的 w
    Vec3f B = T.cross(frag_n).normalized() * det;

    // 此时 TBN 同时具备从 tangent -> object 和 object -> world 的变换
    Mat3f TBN = (Mat3f() << T, B, frag_n).finished();

    LinearColor raw = Samplers::LinearClamp.sampleColor(*mat->norm_slot, frag_uv.x(), frag_uv.y());

    // 将 [0, 1] 的 LinearColor 映射到 [-1, 1]] 的坐标空间，得到切线空间的法向
    Vec3f n_T = Vec3f{raw.R * 2.0f - 1.0f, raw.G * 2.0f - 1.0f, raw.B * 2.0f - 1.0f}.normalized();

    // 这里 frag_n 会被赋值为贴图采样的法向，为着色法线 Shading Normal，以扰动后续的所有光照计算。
    frag_n = TBN * n_T;
  }

  // 若启用 Normal Mapping, 则 nl_shading 会自动被扰动。
  const f32 nl_shading = frag_n.dot(uniforms.light_dir_inv);

  LinearColor specular{0};
  if(nl > 0 && mat->enable_spec) {
    Vec3f view_dir = (uniforms.camera_pos - frag_pos).normalized();

    Vec3f h = (view_dir + uniforms.light_dir_inv).normalized();

    f32 spec = std::powf(std::max(frag_n.dot(h), 0.0f), mat->params.Ns);

    specular = sampleSlot(mat->spec_slot, frag_uv) * spec;
  }

  // 半兰伯特：不受光的 frag 漫反射强度从 [-1,1] 被 wrap 到 [0.5,1] 再到 [0.25,1].
  f32 half_lambert = std::max(nl_shading, 0.0f) * 0.5f + 0.5f;

  f32 diff = half_lambert * half_lambert;

  frag_color = (diffuse * diff + specular) * uniforms.light_color * frag_visibility;

  frag_color.A = frag_alpha;

  return false;
}

} // namespace Medium
