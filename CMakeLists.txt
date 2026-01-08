cmake_minimum_required(VERSION 3.10)
project(Renderer)

# 要求 C++17 标准（对 Eigen 的对齐和现代特性更好）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(external)

# 加入MiniFB项目
add_subdirectory(external/minifb)
option(USE_METAL_API "开启MiniFB的macOS Metal支持" ON)

# 递归地寻找源码
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 生成可执行文件
add_executable(Renderer ${SOURCES})

# 将MiniFB链接进程序
target_link_libraries(${PROJECT_NAME} minifb)

# 如果是 Release 模式，开启高性能优化开关
target_compile_options(${PROJECT_NAME} PRIVATE -O3 -march=native)