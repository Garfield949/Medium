cmake_minimum_required(VERSION 3.16)

project(MediumCanvas DESCRIPTION "CPU Renderer")

# 开启导出 compile_commands.json，以便 clangd 等索引
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# add_subdirectory: 将另一个 CMake 项目纳入当前项目一起配置和编译
# EXCLUDE_FROM_ALL 默认排除构建 SDL，但会自动构建被 Medium 引入的部分
add_subdirectory(external/SDL EXCLUDE_FROM_ALL)
add_subdirectory(external/fmt EXCLUDE_FROM_ALL)

# 源码列表排除解释器 lua.c 和编译器 luac.c
file(GLOB LUA_SOURCES "external/lua/*.c")
list(REMOVE_ITEM LUA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/external/lua/lua.c"
                             "${CMAKE_CURRENT_SOURCE_DIR}/external/lua/luac.c")
# Lua 静态编译进二进制
add_library(lua_static STATIC ${LUA_SOURCES})

# 优化选项，让源文件在链接阶段也能参与内联，只对 release 构建生效
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

# 生成可执行文件，并顺便指定源文件
add_executable(${PROJECT_NAME} ${IMGUI_SOURCES})

# CMake 会为含 CMakeLists.txt 的子目录分配一个构建文件夹
add_subdirectory(Medium)

# target_ 开头的命令操作的是一个已经存在的 CMake target, 需放在 add_executable 之后。

# 向构建目标追加源文件，ImGui 静态编译
target_sources(${PROJECT_NAME} PRIVATE
    external/DearImGui/imgui.cpp
    external/DearImGui/imgui_draw.cpp
    external/DearImGui/imgui_tables.cpp
    external/DearImGui/imgui_widgets.cpp
    external/DearImGui/backend/imgui_impl_sdl3.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 23          # 指定项目使用 C++23
    CXX_STANDARD_REQUIRED ON # 若编译器较旧不支持，不允许静默降级，编译直接报错
    CXX_EXTENSIONS OFF       # 不允许开启编译器扩展
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall            # Warning All, 开启大部分编译器警告
    -Wextra          # 检查更细致的语法问题
    -pedantic-errors # 强制遵守 ISO C++ 标准
)

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Medium)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/DearImGui)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/Eigen)

# 定义宏，记录项目的根路径，用于程序内寻找模型文件
target_compile_definitions(${PROJECT_NAME} PRIVATE PROJECT_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/")

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 fmt::fmt lua_static)

# 开启高性能优化开关
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:-O3>           # 高级别优化
    $<$<CONFIG:Release>:-march=native> # 根据当前机器 CPU 启用对应指令集和架构优化
    $<$<CONFIG:Debug>:-O0>             # 禁用优化
    $<$<CONFIG:Debug>:-g>              # 启用调试信息
)