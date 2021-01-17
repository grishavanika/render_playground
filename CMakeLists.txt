# Require 3.11+ for FetchContent* support
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(render_playground)

include(CheckCXXCompilerFlag)
include(GenerateExportHeader)
include(CTest)
include(tools/cmake/utils.cmake)
include(tools/cmake/clang_msvc_integration.cmake)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

detect_compilers()
set_cpp_latest_standard()

# Remove unneeded configurations
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

if (clang_on_msvc)
    make_clang_work_on_windows()
endif()

if (GNU OR clang OR clang_on_msvc)
    add_compile_options(
        # MSVC <regex>
        -Wno-sign-compare)
endif()

# third party libraries
add_subdirectory(third_party)

# libraries
add_subdirectory(src/model)
# exe
add_subdirectory(src/app)
add_subdirectory(src/export)
add_subdirectory(src/app_vulkan)

# tests

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "app")
