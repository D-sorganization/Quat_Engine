cmake_minimum_required(VERSION 3.20)

project(QuatEngine
    VERSION 1.0.0
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

# ── Build Options ────────────────────────────────────────────────────────────
option(QE_BUILD_DEMO "Build the demo application (requires SDL2 and OpenGL)" ON)

# ── Dependencies ─────────────────────────────────────────────────────────────
if(QE_BUILD_DEMO)
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
endif()

# ── Math Library (Header-Only) ───────────────────────────────────────────────
add_library(qe_math INTERFACE)
target_include_directories(qe_math INTERFACE ${CMAKE_SOURCE_DIR}/src)

# ── Core Library (Header-Only) ───────────────────────────────────────────────
add_library(qe_core INTERFACE)
target_include_directories(qe_core INTERFACE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(qe_core INTERFACE qe_math)

# ── Game Library ─────────────────────────────────────────────────────────────
add_library(qe_game INTERFACE)
target_include_directories(qe_game INTERFACE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(qe_game INTERFACE qe_core)

if(QE_BUILD_DEMO)
    # ── Renderer Library ─────────────────────────────────────────────────────
    add_library(qe_renderer INTERFACE)
    target_include_directories(qe_renderer INTERFACE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(qe_renderer INTERFACE
        qe_core
        SDL2::SDL2-static
        SDL2::SDL2main
        OpenGL::GL
    )

    # ── Input Library ────────────────────────────────────────────────────────
    add_library(qe_input INTERFACE)
    target_include_directories(qe_input INTERFACE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(qe_input INTERFACE SDL2::SDL2-static)

    # ── Demo Application ──────────────────────────────────────────────────────
    add_executable(qe_demo src/main.cpp)
    target_link_libraries(qe_demo PRIVATE qe_renderer qe_game qe_input)

    # Copy shaders to build directory
    add_custom_command(TARGET qe_demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:qe_demo>/shaders
    )
endif()

# ── Tests ────────────────────────────────────────────────────────────────────
enable_testing()

add_executable(test_math tests/test_math.cpp)
target_link_libraries(test_math PRIVATE qe_core)
add_test(NAME MathTests COMMAND test_math)

add_executable(test_game tests/test_game.cpp)
target_link_libraries(test_game PRIVATE qe_game)
add_test(NAME GameTests COMMAND test_game)

add_executable(test_wave tests/test_wave.cpp)
target_link_libraries(test_wave PRIVATE qe_game)
add_test(NAME WaveTests COMMAND test_wave)

add_executable(test_behavior tests/test_behavior.cpp)
target_link_libraries(test_behavior PRIVATE qe_game)
add_test(NAME BehaviorTests COMMAND test_behavior)

add_executable(test_particles tests/test_particles.cpp)
target_link_libraries(test_particles PRIVATE qe_core)
add_test(NAME ParticleTests COMMAND test_particles)

add_executable(test_powerup tests/test_powerup.cpp)
target_link_libraries(test_powerup PRIVATE qe_game)
add_test(NAME PowerUpTests COMMAND test_powerup)

add_executable(test_weapons tests/test_weapons.cpp)
target_link_libraries(test_weapons PRIVATE qe_game)
add_test(NAME WeaponTests COMMAND test_weapons)

# ── Status ───────────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "QuatEngine v${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "")
