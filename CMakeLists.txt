cmake_minimum_required(VERSION 3.10)

project(Medium)

# C++17（对 Eigen 的对齐和现代特性更好）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# 生成可执行文件
add_executable(${PROJECT_NAME}
        Medium/Base/Math.h
        Medium/Base/Types.h
        Medium/Pipeline/RenderTarget.h
        Medium/Base/Color.h
        Medium/Resources/Vertex.h
        Medium/Resources/Mesh.h
        Medium/Scene/Camera.h
        Medium/Base/Geometry.h
        Medium/Scene/Transform.h
        Medium/Scene/Camera.cpp
        Medium/main.cpp
        Medium/Resources/Mesh.cpp
        Medium/Scene/Transform.cpp
        Medium/Base/Timer.h
        Medium/Base/Timer.cpp
        Medium/Base/Input.cpp
        Medium/Base/Input.h
        Medium/Base/Controller.h
        Medium/Base/Controller.cpp
        Medium/App.cpp
        Medium/App.h
        Medium/Scene/Node.cpp
        Medium/Scene/Node.h
        Medium/Scene/MeshInstance.h
        Medium/Pipeline/RenderContext.h
        Medium/Scene/MeshInstance.cpp
        Medium/Resources/Texture.h
        Medium/Resources/Material.h
        Medium/Resources/Forwards.h
        Medium/Base/Color.h
        Medium/Base/SubPixel.h
        Medium/Pipeline/Pipeline.h
        Medium/Pipeline/Pipeline.cpp
        Medium/Base/Config.h
        Medium/Pipeline/Shader/BlinnPhong.h
        Medium/Pipeline/Shader/BlinnPhong.cpp
        Medium/Pipeline/Shader/ShaderType.h
        Medium/Pipeline/Clip.h
        Medium/Resources/AssetCache.cpp
        Medium/Resources/AssetCache.h
        Medium/Base/Meta.h
        Medium/Resources/TextureSlot.h
        Medium/Base/Util.h
        Medium/Resources/Material.cpp
        Medium/Pipeline/ShadowMap.cpp
        Medium/Pipeline/ShadowMap.h
        Medium/Scene/Light/Light.h
        Medium/Scene/Light/Light.cpp
        Medium/Scene/Light/DirectionalLight.h
        Medium/Scene/Light/DirectionalLight.cpp
        Medium/Pipeline/Shader/ShadowShader.cpp
        Medium/Pipeline/Shader/ShadowShader.h
        Medium/Pipeline/Shader/Varying.h
        Medium/Scene/Scene.cpp
        Medium/Scene/Scene.h
)

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external)

# 将项目的 include 目录加入全局搜索路径
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Medium)

# 定义宏，记录项目的根路径，用于程序内寻找模型文件
target_compile_definitions(${PROJECT_NAME} PRIVATE PROJECT_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/")

target_compile_definitions(${PROJECT_NAME} PRIVATE FMT_HEADER_ONLY)

target_link_libraries(${PROJECT_NAME} PRIVATE minifb)

target_link_libraries(${PROJECT_NAME} PRIVATE lua_static)

# 开启高性能优化开关，用于支持 Eigen
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native>
    $<$<CONFIG:Debug>:-O0 -g>
)

# 优化选项，让源文件在链接阶段也能参与内联
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)