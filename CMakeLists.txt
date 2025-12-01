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
    src/Engine/Renderer/Camera.cpp
    src/Engine/Renderer/Shader.cpp
    src/Game/World/Chunk.cpp
)

# --------------------------------------------------------------------
# Includes & Links
# --------------------------------------------------------------------
add_subdirectory(external/glfw-3.4)
add_library(glad external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    external/glad/include
    external/glm
    external/glfw-3.4/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad
    opengl32
)

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