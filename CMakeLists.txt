cmake_minimum_required(VERSION 3.21)
project(MCPP LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------------------------------------------------------
# Options
# --------------------------------------------------------------------
option(ENABLE_ASAN "Enable AddressSanitizer (Debug only)" OFF)
option(ENABLE_LTO "Enable Link Time Optimization (Release only)" ON)

# --------------------------------------------------------------------
# Source files
# --------------------------------------------------------------------
add_executable(${PROJECT_NAME}
        src/main.cpp
        src/Game/MinecraftApp.cpp
        src/Game/Player/PlayerController.cpp
        src/Engine/ImGui/ImGuiLayer.cpp
        src/Engine/Renderer/Camera.cpp
        src/Engine/Renderer/Shader.cpp
        src/Engine/Physics/Collision.cpp
        src/Game/World/Generation/Chunk.cpp
        src/Game/World/ChunkManager.cpp
        src/Game/Rendering/TextureAtlas.cpp
        src/Game/World/Block/BlockDatabase.cpp
        src/Engine/Renderer/Frustum.cpp
        src/Engine/Renderer/Frustum.h
        src/Game/World/Generation/Surface.h
        src/Game/World/Generation/Noise.h
        src/Game/World/Generation/Surface.cpp
        src/Game/World/Generation/SurfaceManager.h
        src/Game/World/Generation/SurfaceManager.cpp
        src/Game/World/Meshing/Greedy.h
        src/Game/World/Meshing/Greedy.cpp
)

# --------------------------------------------------------------------
# Includes & Links
# --------------------------------------------------------------------
include(FetchContent)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)

if (EXISTS "${CMAKE_SOURCE_DIR}/vendor/glfw/CMakeLists.txt")
    add_subdirectory(vendor/glfw)
else()
    message(STATUS "Fetching GLFW (vendor folder missing CMake files)")
    FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.4
    )
    FetchContent_MakeAvailable(glfw)
endif()

add_library(glad vendor/glad/src/glad.c)
target_include_directories(glad PUBLIC vendor/glad/include)

set(GLM_DIR "${CMAKE_SOURCE_DIR}/vendor/glm")
if (NOT EXISTS "${GLM_DIR}/glm/glm.hpp")
    message(STATUS "Fetching GLM (vendor folder missing headers)")
    FetchContent_Declare(
            glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG 1.0.1
    )
    FetchContent_MakeAvailable(glm)
    set(GLM_DIR "${glm_SOURCE_DIR}")
endif()

set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/vendor/imgui")
if (NOT EXISTS "${IMGUI_DIR}/imgui.cpp")
    message(STATUS "Fetching ImGui (vendor folder missing sources)")
    FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG docking
    )
    FetchContent_MakeAvailable(imgui)
    set(IMGUI_DIR "${imgui_SOURCE_DIR}")
endif()

set(PROJECT_INCLUDES
        src
        vendor/glad/include
        ${GLM_DIR}
        vendor/glfw/include
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        vendor/stb
        vendor/nlohmann
)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDES})

target_sources(${PROJECT_NAME} PRIVATE
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        glad
)

if (UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE GL dl pthread X11 Xi Xrandr Xinerama Xcursor)
elseif (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        IMGUI_IMPL_OPENGL_LOADER_GLAD
)

# --------------------------------------------------------------------
# Compiler options / sanitizers / LTO
# --------------------------------------------------------------------
if(ENABLE_ASAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/fsanitize=address>
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fsanitize=address,undefined>
    )
    target_link_options(${PROJECT_NAME} PRIVATE
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fsanitize=address,undefined>
    )
endif()

if(ENABLE_LTO AND (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo"))
    include(CheckIPOSupported)
    check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
    if(IPO_SUPPORTED)
        set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()

# --------------------------------------------------------------------
# Copy assets folder next to executable after every build
# --------------------------------------------------------------------
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
        COMMENT "Copying assets to output directory"
)

# Also copy to binary dir (helps when running directly from CLion)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "${CMAKE_BINARY_DIR}/assets"
)