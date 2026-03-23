cmake_minimum_required(VERSION 3.10)

project(Medium)

# C++17（对 Eigen 的对齐和现代特性更好）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 加入MiniFB项目
set(MINIFB_BUILD_EXAMPLES FALSE CACHE BOOL "" FORCE)
add_subdirectory(external/minifb)
option(USE_METAL_API "开启 MiniFB 的 macOS Metal 支持" ON)

# 源码列表排除解释器 lua.c 和编译器 luac.c
file(GLOB LUA_SOURCES "external/lua/*.c")
list(REMOVE_ITEM LUA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/external/lua/lua.c"
                             "${CMAKE_CURRENT_SOURCE_DIR}/external/lua/luac.c")

# 将 Lua 静态编译进二进制
add_library(lua_static STATIC ${LUA_SOURCES})

# 递归获取 Medium 目录下所有的源文件
file(GLOB_RECURSE SOURCES "Medium/*.cpp" "Medium/*.h")

# 生成可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/lua)

# 将项目目录加入全局包含路径
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Medium)

# 定义宏，记录项目的根路径，用于程序内寻找模型文件
target_compile_definitions(${PROJECT_NAME} PRIVATE PROJECT_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/")

target_compile_definitions(${PROJECT_NAME} PRIVATE FMT_HEADER_ONLY)

target_link_libraries(${PROJECT_NAME} PRIVATE minifb lua_static)

# 开启高性能优化开关，用于支持 Eigen
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native>
    $<$<CONFIG:Debug>:-O0 -g>
)

# 优化选项，让源文件在链接阶段也能参与内联
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)