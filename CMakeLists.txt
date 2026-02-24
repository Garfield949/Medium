cmake_minimum_required(VERSION 3.10)
project(Medium)

# C++17（对 Eigen 的对齐和现代特性更好）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 加入MiniFB项目
add_subdirectory(external/minifb)
option(USE_METAL_API "开启 MiniFB 的 macOS Metal 支持" ON)

# 定义宏，记录项目的根路径，用于程序内寻找模型文件
add_definitions(-DPROJECT_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/")

# 定义宏，以在单文件内展开具体实现，防止多重定义
add_definitions(-DFMT_HEADER_ONLY)

# 生成可执行文件
add_executable(${PROJECT_NAME}
        Medium/Core/Math/Math.h
        Medium/Core/Types.h
        Medium/Pipeline/RenderTarget.h
        Medium/Core/Color.h
        Medium/Core/Primitivies/Vertex.h
        Medium/Resources/Mesh.h
        Medium/Scene/Camera.h
        Medium/Pipeline/RenderConfig.h
        Medium/Pipeline/Rasterizer.h
        Medium/Core/Math/Geometry.h
        Medium/Pipeline/Shader/Shaders.h
        Medium/Scene/Transform.h
        Medium/Pipeline/Rasterizer.cpp
        Medium/Scene/Camera.cpp
        Medium/main.cpp
        Medium/Resources/Mesh.cpp
        Medium/Scene/Transform.cpp
        Medium/Core/Timer.h
        Medium/Core/Timer.cpp
        Medium/Core/Input.cpp
        Medium/Core/Input.h
        Medium/Core/Controller.h
        Medium/Core/Controller.cpp
        Medium/Application.cpp
        Medium/Application.h
        Medium/Scene/Node.cpp
        Medium/Scene/Node.h
        Medium/Scene/MeshInstance.h
        Medium/Pipeline/Shader/IShader.h
        Medium/Pipeline/RenderContext.h
        Medium/Scene/MeshInstance.cpp
        Medium/Resources/Texture.cpp
        Medium/Resources/Texture.h
        Medium/Resources/Material.cpp
        Medium/Resources/Material.h
        Medium/Core/Forwards.h
)

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external)

# 将项目的 include 目录加入全局搜索路径
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Medium)

# 将MiniFB链接进程序
target_link_libraries(${PROJECT_NAME} minifb)

# 开启高性能优化开关，用于支持 Eigen
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native>
    $<$<CONFIG:Debug>:-O0 -g>
)

# 优化选项，让源文件在链接阶段也能参与内联
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)