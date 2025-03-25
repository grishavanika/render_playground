# Require 3.11+ for FetchContent* support
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(render_playground)

include(CheckCXXCompilerFlag)
include(GenerateExportHeader)
include(CTest)
include(cmake_utils.cmake)
include(cmake_clang_msvc.cmake)

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

# third party libs ----------------------------
# assimp
add_library(Assimp_Integrated INTERFACE)
find_package(assimp CONFIG REQUIRED)
target_link_libraries(Assimp_Integrated INTERFACE assimp::assimp)

# dx11
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
find_package(DirectX11 REQUIRED)
message("DirectX11: ${DirectX11_LIBRARY}")
add_library(DX11_Integrated INTERFACE)
target_include_directories(DX11_Integrated INTERFACE ${DirectX11_INCLUDE_DIR})
target_link_libraries(DX11_Integrated INTERFACE ${DirectX11_LIBRARY})

# glm
add_library(glm_Interface INTERFACE)
find_package(glm CONFIG REQUIRED)
target_compile_definitions(glm_Interface
  INTERFACE GLM_ENABLE_EXPERIMENTAL)
target_link_libraries(glm_Interface INTERFACE glm::glm-header-only)

# imgui
include(FetchContent)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        7674cbc9b25668dcbcc0ffd35b126b094c978e89
)
FetchContent_MakeAvailable(imgui)

add_library(ImGui_Core
    ${imgui_SOURCE_DIR}/imconfig.h
    ${imgui_SOURCE_DIR}/imgui.h
    ${imgui_SOURCE_DIR}/imgui_internal.h
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp)

target_include_directories(ImGui_Core PUBLIC ${imgui_SOURCE_DIR})
target_include_directories(ImGui_Core PUBLIC ${imgui_SOURCE_DIR}/backends)

add_library(ImGui_Cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.h
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp)

target_include_directories(ImGui_Cpp PUBLIC ${imgui_SOURCE_DIR}/misc/cpp)
target_link_libraries(ImGui_Cpp PRIVATE ImGui_Core)

# sdl/opengl3/GL3W
add_library(ImGui_Impl
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.h
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.h
    )

target_include_directories(ImGui_Impl PUBLIC ${imgui_SOURCE_DIR}/examples)
target_link_libraries(ImGui_Impl PRIVATE ImGui_Core)

add_library(ImGui_Integrated INTERFACE)
target_link_libraries(ImGui_Integrated INTERFACE
    ImGui_Core
    ImGui_Impl
    ImGui_Cpp)

set_target_properties(ImGui_Core PROPERTIES FOLDER third_party)
set_target_properties(ImGui_Impl PROPERTIES FOLDER third_party)
set_target_properties(ImGui_Cpp PROPERTIES FOLDER third_party)

# outcome
add_library(outcome INTERFACE)
find_package(outcome CONFIG REQUIRED)
target_link_libraries(outcome INTERFACE outcome::hl)

# stb
add_library(stb_image_Integrated INTERFACE)
find_package(Stb REQUIRED)
target_include_directories(stb_image_Integrated INTERFACE ${Stb_INCLUDE_DIR})

# winio
include(FetchContent)
include(CMakePrintHelpers)

FetchContent_Declare(
  win_io
  GIT_REPOSITORY https://github.com/grishavanika/win_io.git
  GIT_TAG        d0e067621798feace84b8f9f3e2e4935e0183c7b
)
# FetchContent_MakeAvailable can't be used
# since win_io is not FetchContent-friendly
# (add_subdirectory will add more then win_io library)
FetchContent_Populate(win_io)
cmake_print_variables(win_io_SOURCE_DIR)

add_library(win_io INTERFACE)
target_include_directories(win_io INTERFACE
    "${win_io_SOURCE_DIR}/src/win_io/include/win_io")

# -----------------------------------------

# exe
add_subdirectory(src)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "app")
