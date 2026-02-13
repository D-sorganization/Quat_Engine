cmake_minimum_required(VERSION 3.20)

project(ForceFieldEngine
    VERSION 0.1.0
    DESCRIPTION "3D game engine with quaternion-based rotation and modern rendering"
    LANGUAGES CXX
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

# ── Math Library (Header-Only) ───────────────────────────────────────────────
add_library(ffe_math INTERFACE)
target_include_directories(ffe_math INTERFACE
    ${CMAKE_SOURCE_DIR}/src
)

# ── Core Library (Header-Only) ───────────────────────────────────────────────
add_library(ffe_core INTERFACE)
target_include_directories(ffe_core INTERFACE
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(ffe_core INTERFACE ffe_math)

# ── Tests ────────────────────────────────────────────────────────────────────
enable_testing()

add_executable(test_math tests/test_math.cpp)
target_link_libraries(test_math PRIVATE ffe_core)

add_test(NAME MathTests COMMAND test_math)

# ── Main Demo (Phase 2 — placeholder) ───────────────────────────────────────
# add_executable(ffe_demo src/main.cpp)
# target_link_libraries(ffe_demo PRIVATE ffe_core)
# TODO: Link SDL2, OpenGL in Phase 2

# ── Install ──────────────────────────────────────────────────────────────────
# Not yet needed — development only

message(STATUS "")
message(STATUS "ForceFieldEngine v${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "")
