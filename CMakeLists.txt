cmake_minimum_required(VERSION 3.21)

project(Medium DESCRIPTION "CPU Renderer")

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

# 递归获取 Medium 目录下所有的源文件
file(GLOB_RECURSE MEDIUM_SOURCES "Medium/*.cpp" "Medium/*.h" "Medium/*.hpp")

# 生成可执行文件
add_executable(${PROJECT_NAME} ${MEDIUM_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20          # 指定项目使用 C++20
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

# 定义宏，记录项目的根路径，用于程序内寻找模型文件
target_compile_definitions(${PROJECT_NAME} PRIVATE PROJECT_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/")

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 fmt::fmt lua_static)

# 开启高性能优化开关，用于支持 Eigen
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native>
    $<$<CONFIG:Debug>:-O0 -g>
)

# 优化选项，让源文件在链接阶段也能参与内联
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
