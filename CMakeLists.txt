cmake_minimum_required(VERSION 3.10)
project(renderer)

# C++17（对 Eigen 的对齐和现代特性更好）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 加入MiniFB项目
add_subdirectory(external/minifb)
option(USE_METAL_API "开启MiniFB的macOS Metal支持" ON)

# 生成可执行文件
add_executable(renderer src/main.cpp)

# 设置包含目录
target_include_directories(renderer PRIVATE external)

# 将MiniFB链接进程序
target_link_libraries(${PROJECT_NAME} minifb)

# 开启高性能优化开关，用于支持 Eigen
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native>
    $<$<CONFIG:Debug>:-O0 -g>
)