#include <vector>
#include <chrono>

// 定义此宏，以在单文件内展开 tiny_obj_loader 的实现，防止多重定义
#define TINYOBJLOADER_IMPLEMENTATION
#include <TinyObjLoader.h>
#include <MiniFB.h>

#include "Types.h"
#include "Vertex.h"
#include "Framebuffer.h"
#include "RenderConfig.h"
#include "Camera.h"

#define MEDIUM_DEBUG(x) std::cout << "[Line " << __LINE__ << "] " << #x << " = " << (x) << std::endl

using namespace Medium;

void Bresenham(int x0, int y0, int x1, int y1, RenderTarget &buffer, Color color) {
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    bool steep = dy > dx;
    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    dx = x1 - x0;
    dy = std::abs(y1 - y0);
    int error = dx / 2;
    int ystep = (y0 < y1) ? 1 : -1;
    int y = y0;
    if (steep) {
        for (int x = x0; x <= x1; ++x) {
            buffer.setColor(y, x, color);
            error -= dy;
            if (error < 0) {
                y += ystep;
                error += dx;
            }
        }
    } else {
        for (int x = x0; x <= x1; ++x) {
            buffer.setColor(x, y, color);
            error -= dy;
            if (error < 0) {
                y += ystep;
                error += dx;
            }
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
    // TODO：Eigen 的绕轴旋转是右手性的，这里angle没加负号也正常表现为左手性顺时针旋转是为啥？
    model.translate(translation)
        .rotate(Eigen::AngleAxisf(Medium::radians(angle), axis.normalized()))
        .scale(scale);

    return model.matrix();
}

void readObj(const std::string &filePath, std::vector<Vertex> &verts) {
    using namespace tinyobj;
    
    // OBJ 读取器
    ObjReader reader;
    ObjReaderConfig config;
    config.mtl_search_path = "";
    config.triangulate = true;

    // 错误检测
    if (!reader.ParseFromFile(filePath, config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error();
        }
        exit(1);
    }
    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    // attrib_t 存储实际的顶点数据（坐标、法线、纹理等）
    // 注意：attrib_t 的索引数组 std::vector<real_t> vertices 存放的是平铺
    // 的数据，即 x0, y0, z0, x1, y1, z1, ...
    auto& attrib = reader.GetAttrib();
    MEDIUM_DEBUG(attrib.vertices.size());
    MEDIUM_DEBUG(attrib.normals.size());
    MEDIUM_DEBUG(attrib.texcoords.size());
    // shape_t 有 Mesh、Lines、Points。但大多数模型都只以 Mesh 来表示
    // shape_t 里的 Mesh 类型存放顶点索引
    // 一个模型可能有多个 shape。比如头、身、四肢，每个 shape 有独立的 mesh
    // 所以 shapes 的大小很可能是 1
    auto& shapes = reader.GetShapes();

    // 计算总顶点数，提前分配空间（由于是顶点汤 Vertex Soup，不复用顶点，顶点数等于索引数）
    size_t total_vertices = 0;
    for(auto & shape : shapes)
        total_vertices += shape.mesh.indices.size();
    verts.reserve(total_vertices);

    // 遍历 shape
    for(const auto & shape : shapes) {
        // 获取 shape 的 mesh 的面片数
        const auto num_faces = shape.mesh.num_face_vertices.size();
        for(size_t i = 0; i < num_faces; ++ i) {
            // 每 3 个索引为一个三角面，故计算下标偏移
            auto offset = i * 3;
            // 组装为一个三角索引数组
            const index_t tri_indices[] = {
                shape.mesh.indices[offset],
                shape.mesh.indices[offset + 1],
                shape.mesh.indices[offset + 2]
            };
            // 遍历三角面每个顶点的索引 index_t
            for(const auto & idx : tri_indices) {
                // Lambda，根据索引类型组装值并返回
                auto fetch = [&](const std::vector<real_t>& data, int idx_type, int stride) -> Vec3f {
                    // 当 idx_type = -1 即法线或纹理不存在时，返回零向量
                    if(idx_type < 0)
                        return Vec3f::Zero();
                    Vec3f res;
                    for (int j = 0; j < stride; ++j) {
                        res[j] = data[stride * idx_type + j];
                    }
                    return res;
                };
                verts.emplace_back(fetch(attrib.vertices, idx.vertex_index, 3),
                                   fetch(attrib.normals, idx.normal_index, 3),
                                   fetch(attrib.texcoords, idx.texcoord_index, 2).head<2>());
            }
        }
    }

}

int main() {

    RenderConfig cfg{800, 800};

    Camera camera;
    camera.setAspectRatio(cfg.vpRatio());

    std::vector<Vertex> vertices;

    readObj("../../models/dragon.obj", vertices);

    // 创建 MiniFB 窗口实例
    mfb_window *window = mfb_open_ex("Medium Canvas", cfg.vp_width, cfg.vp_height, WF_RESIZABLE);
    if (!window) {
        return -1;
    }

    // 创建缓冲区实例
    RenderTarget buffer(cfg.vp_width, cfg.vp_height);

    // 模型旋转角度
    f32 angle = 90;

    // 线条颜色
    Color color{79, 147, 184};

    // 定义时间点
    auto last_time = std::chrono::high_resolution_clock::now();
    double frame_count = 0;
    double second_accumulator = 0; // 累计经过的时间

    do {
        // 计时
        auto current_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = current_time - last_time;
        double delta_time = diff.count();
        last_time = current_time;
        // FPS 计算与显示逻辑
        second_accumulator += delta_time;
        frame_count++;
        // 每经过 1 秒更新一次 FPS 显示
        if (second_accumulator >= 1.0) {
            double fps = frame_count / second_accumulator;
            double ms_per_frame = (second_accumulator / frame_count) * 1000.0;
            std::cout << "\r[FPS]: " << fps << " | [Frame Time]: " << ms_per_frame << " ms" << std::flush;
            // 重置计数
            second_accumulator = 0;
            frame_count = 0;
        }

        // MiniFB 状态类型
        mfb_update_state state;

        // 填充背景颜色
        buffer.clearColor(cfg.clear_color);

        // 获取矩阵
        Mat4f m = getModelMatrix(Vec3f{0, 0, 2},1, Vec3f::UnitY(), ++angle);
        Mat4f v = camera.matView();
        Mat4f p = camera.matProj();
        Mat4f mvp = p * v * m;

        for(int i = 0; i < vertices.size(); i+=3) {
            // 将每三个顶点组装为一个三角形数组
            Vec4f tri[] = {
                mvp * vertices[i].pos.homogeneous(),
                mvp * vertices[i+1].pos.homogeneous(),
                mvp * vertices[i+2].pos.homogeneous()
            };

            // 透视除法 & clip 映射到 ndc
            for (auto &vert: tri) {
                vert /= vert.w();
            }

            // ndc 映射到 viewport
            // 对 y 进行了翻转，因为最终的屏幕坐标原点在左上角
            // 末尾加了 0.5，是因为 C++ 不是精确四舍五入，而是向零截断。比如 10.7 会被截断为 10
            // 多加的 0.5 能使顶点转换为整型时有正确的舍入行为。
            for (auto &vert : tri) {
                vert.x() = 0.5f * static_cast<float>(cfg.vp_width) *  (vert.x() + 1.0f) + 0.5f;
                vert.y() = 0.5f * static_cast<float>(cfg.vp_height) * (1.0f - vert.y()) + 0.5f;
            }

            // 线框模式
            Bresenham(tri[0].x(), tri[0].y(), tri[1].x(), tri[1].y(), buffer, color);
            Bresenham(tri[1].x(), tri[1].y(), tri[2].x(), tri[2].y(), buffer, color);
            Bresenham(tri[2].x(), tri[2].y(), tri[0].x(), tri[0].y(), buffer, color);
        }

        // buffer 参数类型为 void* ，故必须将 const 卸去
        state = mfb_update_ex(window, const_cast<u32*>(buffer.frameBuf()), cfg.vp_width, cfg.vp_height);

        // MiniFB 状态检测
        if (state != STATE_OK) {
            window = nullptr;
            break;
        }
    } while(mfb_wait_sync(window));

    return 0;
}