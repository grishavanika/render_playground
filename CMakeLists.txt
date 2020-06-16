# Require 3.11+ for FetchContent* support
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(learn_rendering)

include(CheckCXXCompilerFlag)
include(GenerateExportHeader)
include(CTest)
include(tools/cmake/utils.cmake)
include(tools/cmake/clang_msvc_integration.cmake)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

detect_compilers()
set_cpp17_standard()

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
# add_subdirectory(third_party)

# libraries

# exe
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/tools/cmake")
add_subdirectory(src/learn_rendering)

# tests
