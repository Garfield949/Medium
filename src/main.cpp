#include <iostream>
#include <MiniFB.h>

#include "Types.h"
#include "Framebuffer.h"

using namespace Medium;

// 窗口大小
const unsigned WIDTH_WINDOW = 800;
const unsigned HEIGHT_WINDOW = 800;

// 设置相机参数
const Vec3f POS_CAM{0, 0, 0};
const Vec3f UP_REF{0,1,0};

// 直线绘制
void Bresenham(int x0, int y0, int x1, int y1, Framebuffer &buffer, Color color) {
    // 确保x为最大位移方向
    bool steep = false;
    if (std::abs(x0-x1)<std::abs(y0-y1)) {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    // 确保计算方向为正向
    if(x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int dX = std::round(fabs(x1 - x0));
    int dY = std::round(fabs(y1 - y0));
    int delta = dX - 2 * dY;

    int dStepUp = 2 * (dX - dY);
    int dStepDown = -2 * dY;

    int x = x0, y = y0;

    for(int i = x; i <= x1; i++) {
        !steep ? buffer.setPixel(i, y, color)
               : buffer.setPixel(y, i, color);
        if(delta < 0) {
            y += (y0<y1?1:-1);
            delta += dStepUp;
        } else {
            delta += dStepDown;
        }
    }
}

// 返回模型变换矩阵
// 参数：旋转轴、角度、平移、缩放
Mat4f getModelMatrix(const Vec3f &translation = Vec3f::Zero(),
                     const f32 &scale = 1,
                     const Vec3f &axis = Vec3f::Zero(),
                     const f32 &angle = 0)
{
    Aff3f model = Aff3f::Identity();

    // Eigen绕轴旋转是右手系的，所以CCW取负相当于CW
    model.translate(translation)
        .rotate(Eigen::AngleAxisf(Medium::radians(-angle), axis.normalized()))
        .scale(scale);

    return model.matrix();
}

// 返回视图变换矩阵
// 目前只包含平移
Mat4f getViewMatrix() {
    Mat4f view = Mat4f::Identity();

    Aff3f translation = Aff3f::Identity();
    translation.translate(-POS_CAM);

    return view * translation.matrix();
}

// 返回投影变换矩阵
Mat4f getProjectionMatrix(const f32 fov_y,
                          const f32 aspect_ratio,
                          const f32 near,
                          const f32 far) {
    Mat4f projection = Mat4f::Zero();
    const f32 zoomy = 1 / tanf(radians(fov_y / 2));
    const f32 zoomx = zoomy / aspect_ratio;
    projection << zoomx, 0, 0, 0,
                  0, zoomy, 0, 0,
                  0, 0, (far+near)/(far-near), (-2*near*far) / (far-near),
                  0, 0, 1, 0;
    return projection;
}

int main() {

    // 正方体顶点，位于模型空间，原点为正方体重心
    const Vec3f cube_verts[] = {
        {1,1,1},{-1,1,1},{-1,-1,1},{1,-1,1},
        {1,1,-1},{-1,1,-1},{-1,-1,-1},{1,-1,-1}
    };

    // 正方体索引数组，每个面分为两个三角形，采用顺时针CW顶点索引
    const Vec3i cube_triangles[] = {
        {0,2,3},{0,1,2},    // 背面
        {1,5,2},{5,6,2},    // 左面
        {1,0,4},{5,1,4},    // 上面
        {4,0,3},{7,4,3},    // 右面
        {2,7,3},{6,7,2},    // 下面
        {5,4,7},{7,6,5}     // 前面
    };

    // 横纵比
    const float aspect_ratio = WIDTH_WINDOW / HEIGHT_WINDOW;

    // 创建 MiniFB 窗口实例
    mfb_window *window = mfb_open_ex("Medium", WIDTH_WINDOW, HEIGHT_WINDOW, WF_RESIZABLE);
    if (!window) {
        return -1;
    }

    // 创建缓冲区实例
    Framebuffer buffer(WIDTH_WINDOW, HEIGHT_WINDOW);

    // 模型旋转角度
    f32 angle = 0;

    Color color{255};

    do {
        mfb_update_state state;

        // 填充背景颜色
        buffer.clearPixel({255});

        // 获取矩阵
        Mat4f m = getModelMatrix(Vec3f{0,0,4},1, Vec3f::UnitY(),angle++);
        Mat4f v = getViewMatrix();
        Mat4f p = getProjectionMatrix(100, aspect_ratio,0.1,100);
        Mat4f mvp = p * v * m;

        // 遍历三角形
        for (auto &tri: cube_triangles) {
            Vec4f verts[] = {
                mvp * cube_verts[tri[0]].homogeneous(),
                mvp * cube_verts[tri[1]].homogeneous(),
                mvp * cube_verts[tri[2]].homogeneous()
            };

            for (auto &vert: verts) {
                vert /= vert.w();
            }

            for (auto &vert : verts) {
                vert.x() = 0.5*WIDTH_WINDOW*(vert.x()+1.0);
                vert.y() = 0.5*HEIGHT_WINDOW*(vert.y()+1.0);
            }

            Bresenham(verts[0].x(), verts[0].y(), verts[1].x(), verts[1].y(), buffer, color);
            Bresenham(verts[1].x(), verts[1].y(), verts[2].x(), verts[2].y(), buffer, color);
            Bresenham(verts[2].x(), verts[2].y(), verts[0].x(), verts[0].y(), buffer, color);
        }

        // buffer 参数类型为 void* ，故必须将 const 卸去
        state = mfb_update_ex(window, const_cast<u32*>(buffer.pixelBuf()), WIDTH_WINDOW, HEIGHT_WINDOW);

        // MiniFB 状态检测
        if (state != STATE_OK) {
            window = nullptr;
            break;
        }
    } while(mfb_wait_sync(window));

    return 0;
}