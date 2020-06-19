set(exe_name lr_app)

# TODO: wrap third-party dependencies into CMake's targets and move
# to third_party folder.
find_package(DirectX11 REQUIRED)

set(shaders_files
    shaders/vs_basic_phong_lighting.hlsl
    shaders/ps_basic_phong_lighting.hlsl
# Not included to avoid compilation
# TODO: check how to include but do not compile.
#    shaders/common_basic_phong_lighting.hlsl
    )
set_source_files_properties(shaders/vs_basic_phong_lighting.hlsl PROPERTIES
    VS_SHADER_ENTRYPOINT          main_vs
    VS_SHADER_TYPE                Vertex
    VS_SHADER_OUTPUT_HEADER_FILE  shaders/vs_basic_phong_lighting.h
    VS_SHADER_VARIABLE_NAME       k_VS)
set_source_files_properties(shaders/ps_basic_phong_lighting.hlsl PROPERTIES
    VS_SHADER_ENTRYPOINT          main_ps
    VS_SHADER_TYPE                Pixel
    VS_SHADER_OUTPUT_HEADER_FILE  shaders/ps_basic_phong_lighting.h
    VS_SHADER_VARIABLE_NAME       k_PS)

set(src_files
    cube_model_hardcoded.cpp
    render_main.cpp
    )
set(header_files
    stub_window.h
    utils.h
    )
add_executable(${exe_name} WIN32 ${src_files} ${header_files} ${shaders_files})

target_compile_definitions(${exe_name} PRIVATE UNICODE _UNICODE)
if (${gcc_on_msvc})
    # Make GCC aware about wmain()
    set_target_properties(${exe_name} PROPERTIES LINK_FLAGS -municode)
endif()

set_all_warnings(${exe_name} PRIVATE)

# Own dependencies.
target_link_libraries(${exe_name} lr_model)

# Generated shaders.
target_include_directories(${exe_name} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
# DX11.
target_include_directories(${exe_name} PUBLIC ${DirectX11_INCLUDE_DIRS})
target_link_libraries(${exe_name} ${DirectX11_LIBRARY})
