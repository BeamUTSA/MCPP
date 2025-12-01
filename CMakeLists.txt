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
    src/Engine/Renderer/Camera.cpp
    src/Engine/Renderer/Shader.cpp
    src/Game/World/Chunk.cpp
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

set(PROJECT_INCLUDES
    src
    vendor/glad/include
    vendor/glm
    vendor/glfw/include
)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad
)

if (UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE GL dl pthread)
elseif (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    _CRT_SECURE_NO_WARNINGS
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
