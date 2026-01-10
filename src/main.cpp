#include <iostream>
#include <cmath>

#include <MiniFB.h>
#include <Eigen/Core>
#include <Eigen/Geometry>

// 类型别名
// Affine3f底层是四维矩阵，包含了常用变换方法
using Aff3f = Eigen::Affine3f;
using Vec3f = Eigen::Vector3f;
using Vec4f = Eigen::Vector4f;
using Vec3i = Eigen::Vector3i;
using Mat4f = Eigen::Matrix4f;
using Mat3f = Eigen::Matrix3f;

struct Color {
    unsigned r,g,b;
};

// 窗口大小
const unsigned WIDTH_WINDOW = 800;
const unsigned HEIGHT_WINDOW = 800;

// 视口尺寸、横纵比
const float WIDTH_VIEWPORT = 800;
const float HEIGHT_VIEWPORT = 600;

// DirectX style 世界坐标系：+z 指向屏幕内部
const Vec3f ORIGIN_WORLD = {0,0,0};
const Vec3f AXIS_X_WORLD = Vec3f::UnitX();
const Vec3f AXIS_Y_WORLD = Vec3f::UnitY();
const Vec3f AXIS_Z_WORLD = Vec3f::UnitZ();

// 设置相机参数
const Vec3f POS_CAM{0, 0, 0};
const Vec3f UP_REF{0,1,0};

unsigned calcIndex(unsigned x, unsigned y) {
    return (unsigned)((HEIGHT_WINDOW - y) * WIDTH_WINDOW + x);
}

void setColor(uint32_t* buffer, unsigned x, unsigned y, const Color &color) {
    if (x >= WIDTH_WINDOW || y >= HEIGHT_WINDOW)
        return;
    unsigned index = calcIndex(x, y);
    buffer[index] = MFB_RGB(color.r, color.g, color.b);
}

void Bresenham(int x0, int y0, int x1, int y1, uint32_t *buffer, Color &color) {
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
        !steep ? setColor(buffer, i, y, color)
               : setColor(buffer, y, i, color);
        if(delta < 0) {
            y += (y0<y1?1:-1);
            delta += dStepUp;
        } else {
            delta += dStepDown;
        }
    }
}

// 度数转弧度
constexpr float radians(float deg) {
    return EIGEN_PI * deg / 180.0f;
}

// 返回模型变换矩阵
// 参数：旋转轴、角度、平移、缩放
Mat4f getModelMatrix(const Vec3f &translation = Vec3f::Zero(),
                     const float &scale = 1,
                     const Vec3f &axis = Vec3f::Zero(),
                     const float &angle = 0)
{
    Aff3f model = Aff3f::Identity();

    // Eigen绕轴旋转是右手系的，所以CCW取负相当于CW
    model.translate(translation)
        .rotate(Eigen::AngleAxisf(radians(-angle), axis.normalized()))
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
Mat4f getProjectionMatrix(const float &fov_y,
                          const float &aspect_ratio,
                          const float &near,
                          const float &far) {
    Mat4f projection = Mat4f::Zero();
    const float zoomy = 1 / tanf(radians(fov_y / 2));
    const float zoomx = zoomy / aspect_ratio;
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

    const float aspect_ratio = WIDTH_WINDOW / HEIGHT_WINDOW;

    mfb_window *window = mfb_open_ex("Renderer", WIDTH_WINDOW, HEIGHT_WINDOW, WF_RESIZABLE);

    if (!window) {
        return -1;
    }

    uint32_t *buffer = new uint32_t[WIDTH_WINDOW * HEIGHT_WINDOW * 4];

    Color clearColor{255,255,255};
    Color lineColor{79,147,184};

    int angle = 45;

    do {
        mfb_update_state state;

        for (int i = 0; i < WIDTH_WINDOW * HEIGHT_WINDOW * 4; i++) {
            buffer[i] = MFB_RGB(clearColor.r, clearColor.g, clearColor.b);
        }

        Mat4f m = getModelMatrix(Vec3f{0,0,4},1, AXIS_Y_WORLD,angle++);
        Mat4f v = getViewMatrix();
        Mat4f p = getProjectionMatrix(90,aspect_ratio,0.1,100);
        Mat4f mvp = p * v * m;

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

            // for (auto &vert: verts) {
            //     std::cout << vert.x() << " " << vert.y() << " " << vert.z() << " " << vert.w() << std::endl;
            // }

            Bresenham(verts[0].x(), verts[0].y(), verts[1].x(), verts[1].y(), buffer, lineColor);
            Bresenham(verts[1].x(), verts[1].y(), verts[2].x(), verts[2].y(), buffer, lineColor);
            Bresenham(verts[2].x(), verts[2].y(), verts[1].x(), verts[1].y(), buffer, lineColor);
        }

        state = mfb_update_ex(window, buffer, WIDTH_WINDOW, HEIGHT_WINDOW);
        if (state != STATE_OK) {
            window = NULL;
            break;
        }
    } while(mfb_wait_sync(window));

    return 0;
}