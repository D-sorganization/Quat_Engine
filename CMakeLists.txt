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
option(QE_ENABLE_COVERAGE "Enable gcov-compatible coverage flags for native tests" OFF)

if(QE_ENABLE_COVERAGE)
    if(MSVC)
        message(FATAL_ERROR "QE_ENABLE_COVERAGE is only supported on GCC/Clang toolchains")
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-O0 -g --coverage)
        add_link_options(--coverage)
    else()
        message(FATAL_ERROR "QE_ENABLE_COVERAGE requires GCC or Clang")
    endif()
endif()

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

# ── TPS Game Library (Header-Only) ──────────────────────────────────────────
add_library(qe_tps INTERFACE)
target_include_directories(qe_tps INTERFACE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(qe_tps INTERFACE qe_game)

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
    add_executable(qe_demo
        src/main.cpp
        src/demo/Bootstrap.cpp
        src/demo/RuntimeSystems.cpp
        src/demo/Rendering.cpp
    )
    target_link_libraries(qe_demo PRIVATE qe_renderer qe_game qe_input)

    # ── TPS Game Application ─────────────────────────────────────────────────
    add_executable(qe_tps_game src/tps_main.cpp)
    target_link_libraries(qe_tps_game PRIVATE qe_renderer qe_tps qe_input)

    # Copy shaders to build directory for both executables
    add_custom_command(TARGET qe_demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:qe_demo>/shaders
    )
    add_custom_command(TARGET qe_tps_game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:qe_tps_game>/shaders
    )
endif()

# ── Tests ────────────────────────────────────────────────────────────────────
enable_testing()

function(qe_add_test target)
    set(options)
    set(oneValueArgs SOURCE)
    set(multiValueArgs LIBS LABELS)
    cmake_parse_arguments(QE_TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT QE_TEST_SOURCE)
        message(FATAL_ERROR "qe_add_test(${target}) requires SOURCE")
    endif()

    add_executable(${target} ${QE_TEST_SOURCE})
    target_link_libraries(${target} PRIVATE ${QE_TEST_LIBS})
    add_test(NAME ${target} COMMAND ${target})

    if(QE_TEST_LABELS)
        set_tests_properties(${target} PROPERTIES LABELS "${QE_TEST_LABELS}")
    endif()
endfunction()

qe_add_test(test_math
    SOURCE tests/test_math.cpp
    LIBS qe_core
    LABELS unit math
)

qe_add_test(test_game
    SOURCE tests/test_game.cpp
    LIBS qe_game
    LABELS unit gameplay
)

qe_add_test(test_wave
    SOURCE tests/test_wave.cpp
    LIBS qe_game
    LABELS integration gameplay
)

qe_add_test(test_runtime_session
    SOURCE tests/test_runtime_session.cpp
    LIBS qe_game
    LABELS unit gameplay
)

qe_add_test(test_behavior
    SOURCE tests/test_behavior.cpp
    LIBS qe_game
    LABELS unit gameplay
)

qe_add_test(test_particles
    SOURCE tests/test_particles.cpp
    LIBS qe_core
    LABELS unit core
)

qe_add_test(test_powerup
    SOURCE tests/test_powerup.cpp
    LIBS qe_game
    LABELS unit gameplay
)

qe_add_test(test_weapons
    SOURCE tests/test_weapons.cpp
    LIBS qe_game
    LABELS integration gameplay
)

qe_add_test(test_math_extended
    SOURCE tests/test_math_extended.cpp
    LIBS qe_core
    LABELS unit math
)

qe_add_test(test_quaternion_properties
    SOURCE tests/test_quaternion_properties.cpp
    LIBS qe_core
    LABELS unit math
)

qe_add_test(test_game_extended
    SOURCE tests/test_game_extended.cpp
    LIBS qe_game
    LABELS integration gameplay
)

qe_add_test(test_math_core
    SOURCE tests/test_math_core.cpp
    LIBS qe_core
    LABELS unit math
)

if(QE_BUILD_DEMO)
    qe_add_test(test_postprocess
        SOURCE tests/test_postprocess.cpp
        LIBS qe_renderer
        LABELS integration render
    )
endif()

# ── TPS Game Tests ───────────────────────────────────────────────────────────

qe_add_test(test_tps_character
    SOURCE tests/test_tps_character.cpp
    LIBS qe_tps
    LABELS unit gameplay tps
)

qe_add_test(test_tps_melee
    SOURCE tests/test_tps_melee.cpp
    LIBS qe_tps
    LABELS unit gameplay tps
)

qe_add_test(test_tps_weapons
    SOURCE tests/test_tps_weapons.cpp
    LIBS qe_tps
    LABELS unit gameplay tps
)

qe_add_test(test_tps_lockon
    SOURCE tests/test_tps_lockon.cpp
    LIBS qe_tps
    LABELS unit gameplay tps
)

qe_add_test(test_tps_jump
    SOURCE tests/test_tps_jump.cpp
    LIBS qe_tps
    LABELS unit gameplay tps
)

qe_add_test(test_tps_animation
    SOURCE tests/test_tps_animation.cpp
    LIBS qe_tps
    LABELS unit gameplay tps
)

qe_add_test(test_tps_levels
    SOURCE tests/test_tps_levels.cpp
    LIBS qe_tps
    LABELS integration gameplay tps
)
set_tests_properties(test_tps_levels PROPERTIES
    ENVIRONMENT "QE_LEVELS_DIR=${CMAKE_SOURCE_DIR}/data/levels"
)

qe_add_test(test_tps_combat
    SOURCE tests/test_tps_combat.cpp
    LIBS qe_tps
    LABELS integration gameplay tps
)

qe_add_test(test_tps_ai
    SOURCE tests/test_tps_ai.cpp
    LIBS qe_tps
    LABELS unit gameplay tps
)

qe_add_test(test_tps_feedback
    SOURCE tests/test_tps_feedback.cpp
    LIBS qe_tps
    LABELS unit gameplay tps
)

qe_add_test(test_tps_gamestate
    SOURCE tests/test_tps_gamestate.cpp
    LIBS qe_tps
    LABELS integration gameplay tps
)
set_tests_properties(test_tps_gamestate PROPERTIES
    ENVIRONMENT "QE_LEVELS_DIR=${CMAKE_SOURCE_DIR}/data/levels"
)

# ── Status ───────────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "QuatEngine v${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "  Coverage:     ${QE_ENABLE_COVERAGE}")
message(STATUS "")
