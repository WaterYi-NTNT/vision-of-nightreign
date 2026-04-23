cmake_minimum_required(VERSION 3.15)
project(VisionOfNightreign VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 配置为 DLL
add_library(${PROJECT_NAME} SHARED
    src/dllmain.cpp
    src/game_handler.cpp
    src/memory.cpp
    src/overlay.cpp
    src/ui_renderer.cpp
)

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/structs
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/minhook/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/kiero
)

# ImGui 源文件
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)
target_sources(${PROJECT_NAME} PRIVATE
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
)

# MinHook
set(MINHOOK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/minhook)

target_sources(${PROJECT_NAME} PRIVATE
    ${MINHOOK_DIR}/src/buffer.c
    ${MINHOOK_DIR}/src/hook.c
    ${MINHOOK_DIR}/src/trampoline.c
    ${MINHOOK_DIR}/src/hde/hde32.c
    ${MINHOOK_DIR}/src/hde/hde64.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${MINHOOK_DIR}/include
)

# Kiero
target_sources(${PROJECT_NAME} PRIVATE
    third_party/kiero/kiero.cpp
)

# Windows 库
target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d11.lib
    dxgi.lib
    d3dcompiler.lib
)

# 编译选项
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE 
        /W4 
        /MP  # 多处理器编译
        /permissive-  # 严格标准
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

# 输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
)

# 后置构建：复制 DLL 到游戏目录（可选）
set(GAME_DIR "D:/Games/Nightreign/mods" CACHE PATH "Game mods directory")
if(EXISTS ${GAME_DIR})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${PROJECT_NAME}>
            ${GAME_DIR}
        COMMENT "Copying DLL to game directory..."
    )
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()