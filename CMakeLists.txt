cmake_minimum_required(VERSION 4.2)

project(QuatEngine
    VERSION 0.2.0
    DESCRIPTION "3D game engine with quaternion-based rotation and SLERP interpolation"
    LANGUAGES C CXX
)

# ── C++ Standard ──────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler Warnings ────────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /WX- /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ── Dependencies (auto-downloaded) ───────────────────────────────────────────
include(FetchContent)

# SDL2 — Window management, input, OpenGL context
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-2.30.10
    GIT_SHALLOW    TRUE
)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_TEST   OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL2)

# OpenGL — provided by the operating system
find_package(OpenGL REQUIRED)

# ── Math Library (Header-Only) ───────────────────────────────────────────────
add_library(qe_math INTERFACE)
target_include_directories(qe_math INTERFACE ${CMAKE_SOURCE_DIR}/src)

# ── Core Library (Header-Only) ───────────────────────────────────────────────
add_library(qe_core INTERFACE)
target_include_directories(qe_core INTERFACE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(qe_core INTERFACE qe_math)

# ── Renderer Library ─────────────────────────────────────────────────────────
add_library(qe_renderer INTERFACE)
target_include_directories(qe_renderer INTERFACE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(qe_renderer INTERFACE
    qe_core
    SDL2::SDL2-static
    SDL2::SDL2main
    OpenGL::GL
)

# ── Demo Application ────────────────────────────────────────────────────────
add_executable(qe_demo src/main.cpp)
target_link_libraries(qe_demo PRIVATE qe_renderer)

# Copy shaders to build directory
add_custom_command(TARGET qe_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:qe_demo>/shaders
)

# ── Tests ────────────────────────────────────────────────────────────────────
enable_testing()

add_executable(test_math tests/test_math.cpp)
target_link_libraries(test_math PRIVATE qe_core)
add_test(NAME MathTests COMMAND test_math)

# ── Status ───────────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "QuatEngine v${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "")
