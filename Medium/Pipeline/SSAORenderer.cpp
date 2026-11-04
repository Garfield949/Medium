///
/// @file   SSAORenderer.cpp
/// @author Garfield949
/// @date   2026/11/1 10:39
/// @brief  
///
#include "SSAORenderer.h"

#include <cmath>
#include <random>
#include <algorithm>

#include "Base/Types.h"
#include "Base/Math/Math.h"
#include "Base/PixelIndex.h"

#include "FrameBuffer.h"

namespace Medium {

SSAORenderer::SSAORenderer() {
  // 伪随机数生成器, Random Number Generator. 构造函数接收一个整数作为种子，生成一个 [0,2^32-1] 的整数。
  std::mt19937 rng{1998};
  
  // 均匀实数分布器，生成指定范围内的随机浮点数，必须配合一个随机数引擎，用法：f32 value = random(rng).
  // 这里限定归一化范围，是为了后续生成在单位半球内的点。
  std::uniform_real_distribution<f32> random{0.0f, 1.0f};

  // 初始化采样核。
  for(i32 i = 0; i < kSampleCount; ++i) {
    const f32 rnd = random(rng);
    // 采样点向量在 XY 平面上的归一化投影半径 r:
    // 1. 为什么投影半径不直接 = rnd: rnd 取值范围是 [0,1], 意味着每个采样点有 50% 的几率落入 r < 0.5 的圆盘内，即可能有 kSampleCount / 2 个采样点会落入
    //    r < 0.5 的范围内。而 r = 0.5 的圆盘面积为 0.25π, 0.5 < r < 1.0 的圆环面积为 0.75π, 即圆心附近更小的面积却分到了 50% 的采样点。我们想要的是采样
    //    点在圆盘内均匀分布，即落入任意半径为 a 的圆盘内的概率等于圆盘面积占比：P(r < a) = πa^2/π = a^2, 即目标为 P(r < a) = a^2.
    // 2. 只需令 r = √rnd -> r^2 = rnd. 则有 P(r < a) = P(r^2 < a^2) = P(rnd < a^2) = a^2 达到目标，最后成立的理由是 rnd 在 [0,1] 内均匀分布。
    // 3. 更符合直觉的理解：rnd 是一个归一化数，r = √rnd 会得到一个大于 rnd 且小于 1 的半径。换句话说，采样范围仍是在单位圆盘内，但改变了圆盘内的分布：原本落在
    //    圆心附近的点被往外推到了更大面积的圆环范围内。
    const f32 r = std::sqrt(rnd);
    // 绕 +Z 的随机弧度制角度，由于 rnd 是均匀分布的，故 phi 也是均匀分布的。
    const f32 phi = 2 * Math::Pi * random(rng);
    // 构造一个在单位圆上的向量，故 dir 的模为 1. 注意到根据几何关系，z = sqrt(1.0f - rnd), 会得到一个大于 1.0f - rnd 且小于 1 的值。换句话说这个计算自然而
    // 然地使构造的向量更多地往法线方向靠近 --- 这恰好符合一个目标：由于采样次数有限，我们希望将计算花在更能影响最终亮度的方向上。
    const Vec3f dir {r * std::cos(phi), r * std::sin(phi), std::sqrt(1.0f - rnd)};
    // 1. 将每个采样点均分在 t = 0 ~ 1 的长度上。由于 t < 1, t^2 会更靠近 0，即物体表面，这样做的目的是将有限的采样次数更多集中在接触位置、缝隙附近。
    // 2. t^2 < 1, 故线性映射 0.1f + 0.9f * t * t 将值映射到 [0.1, 1), 使采样点至少与表面距离 0.1 * radius.
    const f32 t = static_cast<f32>(i) / static_cast<f32>(kSampleCount);
    const f32 scale = 0.1f + 0.9f * t * t;

    sample_offsets[i] = scale * dir;
  }
  
  // 初始化旋转。
  for(auto& rotation: rotations) {
    const f32 angle = 2 * Math::Pi * random(rng);
    rotation = {std::cos(angle), std::sin(angle)};
  }
}

void SSAORenderer::resize(i32 w, i32 h) {
  z_view_.resize(w, h);
  ao_raw_.resize(w, h);
}

void SSAORenderer::render(const FrameBuffer& frame_buffer, const CameraContext& cam_ctx) {
  const i32 w   = z_view_.width() , h   = z_view_.height();
  const f32 w_f = z_view_.widthF(), h_f = z_view_.heightF();

  const auto& depth    = frame_buffer.depthBuffer();
  const auto& mat_proj = cam_ctx.mat_proj;

  const f32 near = mat_proj(2,3), zoomx = mat_proj(0,0), zoomy = mat_proj(1,1);

  // 缓存视空间重建深度。
  // 根据 Camera.cpp 的投影矩阵表示，经透视除法后的 Zndc = near / Zview, 故还原 Zview = near / Zndc.
  for(i32 i = 0; i < w * h; ++i)
    z_view_[i] = depth[i] == FrameBuffer::kDepthClearVal ? FrameBuffer::kDepthClearVal
                                                         : near / depth[i];

  // 重建视空间坐标辅助 lambda.
  auto reconstructViewPos = [&](const i32 x, const i32 y) -> Vec3f {
    const f32 x_ndc = 2.0f * (static_cast<f32>(x) + 0.5f) / w_f - 1.0f;
    const f32 y_ndc = 2.0f * (static_cast<f32>(y) + 0.5f) / h_f - 1.0f;
    
    // 深度在 FrameBuffer 中是以翻转 y 存储的，所以索引也须翻转。
    const f32 z = z_view_[PixelIndex::getFlipped(x, y, w, h)];
    
    return { x_ndc * z / zoomx, y_ndc * z / zoomy, z };
  };

  const f32 radius_squared_inv = 1.0f / (radius * radius);

  for(i32 y = 0; y < h; ++y) {
    for(i32 x = 0; x < w; ++x) {
      const size_t idx = PixelIndex::getFlipped(x, y, w, h);

      // 先清除上一帧的残留。
      ao_raw_[idx] = 1.0f;

      // 背景不需计算 ao.
      if(z_view_[idx] == FrameBuffer::kDepthClearVal) continue;

      // 取得当前屏幕像素的视空间坐标。
      const Vec3f pos = reconstructViewPos(x, y);

      // Adjacent, 相邻的。计算与相邻屏幕像素对应视空间坐标差分的 lambda.
      // 比较左右 / 上下合法视空间坐标与 pos 的深度差值，取变化较小的一侧以降低跨物体轮廓的概率。
      auto adjacent_delta = [&](const i32 x_offset, const i32 y_offset) -> Vec3f {
        Vec3f res = Vec3f::Zero();
        f32 prime_depth_delta = std::numeric_limits<f32>::max();

        // 用一个临时 std::initializer_list 作为遍历容器。
        for(i32 sign: {-1, 1}) {
          const i32 nx = x + sign * x_offset, ny = y + sign * y_offset;
          // 邻居像素超出屏幕边界。
          if(nx < 0 || nx >= w || ny < 0 || ny >= h) continue;
          // 邻居像素是背景。
          if(z_view_[PixelIndex::getFlipped(nx, ny, w, h)] == FrameBuffer::kDepthClearVal) continue;
          // 计算邻居像素视空间坐标与当前坐标差分 delta, 保证总是 pos - XXX, 即得到一个 x 或 y 正方向上增大的向量，确保
          // 后续叉积的方向总是一致。
          const Vec3f delta = (reconstructViewPos(nx, ny) - pos) * sign;

          const f32 z_delta = std::abs(delta.z());
          if(z_delta < prime_depth_delta) {
            prime_depth_delta = z_delta;
            res = delta;
          }
        }
        return res;
      };

      const Vec3f dx = adjacent_delta(1, 0);
      const Vec3f dy = adjacent_delta(0, 1);

      // 得到一个 pos 上总是朝向相机侧 (-Z) 的法线。
      Vec3f N = dy.cross(dx);

      // 1. 若 dx, dy 任意一个为 0, 说明当前屏幕像素在 x 或 y 方向上是孤立的，默认其无遮蔽。
      // 2. 若 dx, dy 共线，则无法计算有效叉积（结果为零），默认其无遮蔽。
      const f32 norm_squared = N.squaredNorm();
      if(norm_squared == 0.0f) continue;

      // 法线归一化。
      N /= std::sqrt(norm_squared);

      // 选一个与法线不接近共线的参考向量构建切线基，若法线的 z 太接近 1 同时选 Z 轴作为参考的话，无法计算有效叉积。
      const Vec3f vec_ref = std::fabs(N.z()) < 0.9f ? Vec3f::UnitZ() : Vec3f::UnitY();

      // 需注意一个问题：同一平面上的点算出的法线相同，那么算出的 T 和 B 也相同，会出现平面上构建出的这些基有着相同的旋转，周围采样
      // 出来的点会显现出相同的分布模式，造成条纹或重复轮廓。
      const Vec3f T = vec_ref.cross(N).normalized();
      const Vec3f B = N.cross(T);

      // y % 4 获得 0 ~ 3 的值，再乘以旋转表宽度 4 + (x % 4) 得到下标 0 ~ 15 对应的旋转参数值。
      const Vec2f& rotation = rotations[(y % 4) * 4 + (x % 4)];
      const f32 cos = rotation.x(), sin = rotation.y();

      // 获得随机旋转后的切线空间。
      const Vec3f T_rotated = T * cos + B * sin;
      const Vec3f B_rotated = B * cos - T * sin;

      f32 occlusion_acc = 0.0f;

      for(const Vec3f& sample: sample_offsets) {
        // 还原采样点在视空间的实际坐标。
        const Vec3f q = pos + radius * (sample.x() * T_rotated + sample.y() * B_rotated + sample.z() * N);

        // 采样点有可能突破近平面。
        if(q.z() <= near) continue;

        const f32 z_inv = 1.0f / q.z();

        // 将采样点 q 从视空间变换到 -> ndc -> 屏幕 uv 空间。
        const f32 u = (q.x() * zoomx * z_inv + 1.0f) * 0.5f;
        const f32 v = (q.y() * zoomy * z_inv + 1.0f) * 0.5f;

        // 不进行钳制，否则可能会反复被边缘像素遮挡。
        if(u < 0.0f || u >= 1.0f || v < 0.0f || v >= 1.0f) continue;

        // 屏幕像素坐标最近邻采样。
        const i32 sx = static_cast<i32>(u * w_f);
        const i32 sy = static_cast<i32>(v * h_f);

        // 获取实际已存在场景集合点的深度。
        const f32 z_stored = z_view_[PixelIndex::getFlipped(sx, sy, w, h)];

        // 采样点位置是背景，无遮挡贡献。
        if(z_stored == FrameBuffer::kDepthClearVal) continue;

        // 若已存在深度小于采样点深度，说明产生遮挡。
        if(z_stored + bias >= q.z()) continue;

        // 重建实际遮挡点的视空间坐标，并得到差分。
        const Vec3f delta = reconstructViewPos(sx, sy) - pos;

        // 只考虑来自 pos 切平面上方的遮挡点。
        if(N.dot(delta) <= bias) continue;

        // 遮挡贡献有距离衰减：离 pos 越近贡献越大，这里用距离的平方省去开方。超出采样半径 radius 的话，1 - |delta|^2 / radius^2
        // 会得到一个负数，则不计入遮挡。
        const f32 weight = std::max(0.0f, 1.0f - delta.squaredNorm() * radius_squared_inv);

        // 权重平方进一步削弱离 pos 较远遮挡点贡献，同时使采样半径附近的衰减更平滑。
        occlusion_acc += weight * weight;
      }
      // 对当前像素的遮挡累加做一个平均，然后转换为可见性。
      ao_raw_[idx] = 1.0f - occlusion_acc / static_cast<f32>(kSampleCount);
    } // for(x)
  } // for(y)
}

} // namespace Medium