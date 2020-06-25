set(exe_name lr_app)

set(shaders_files
    shaders/vs_basic_phong_lighting.hlsl
    shaders/ps_basic_phong_lighting.hlsl
    shaders/vs_lines.hlsl
    shaders/ps_lines.hlsl
    shaders/vs_vertices_only.hlsl
    shaders/ps_vertices_only.hlsl
    shaders/vs_normals.hlsl
    shaders/ps_normals.hlsl
# Not included to avoid compilation
# TODO: check how to include but do not compile.
#    shaders/common_basic_phong_lighting.hlsl
    )
set_source_files_properties(shaders/vs_basic_phong_lighting.hlsl PROPERTIES
    VS_SHADER_ENTRYPOINT          main_vs
    VS_SHADER_TYPE                Vertex
    VS_SHADER_OUTPUT_HEADER_FILE  shaders/vs_basic_phong_lighting.h
    VS_SHADER_VARIABLE_NAME       k_vs_basic_phong_lighting
    VS_SHADER_MODEL               5.0)
set_source_files_properties(shaders/ps_basic_phong_lighting.hlsl PROPERTIES
    VS_SHADER_ENTRYPOINT          main_ps
    VS_SHADER_TYPE                Pixel
    VS_SHADER_OUTPUT_HEADER_FILE  shaders/ps_basic_phong_lighting.h
    VS_SHADER_VARIABLE_NAME       k_ps_basic_phong_lighting
    VS_SHADER_MODEL               5.0)
set_source_files_properties(shaders/vs_lines.hlsl PROPERTIES
    VS_SHADER_ENTRYPOINT          main_vs
    VS_SHADER_TYPE                Vertex
    VS_SHADER_OUTPUT_HEADER_FILE  shaders/vs_lines.h
    VS_SHADER_VARIABLE_NAME       k_vs_lines
    VS_SHADER_MODEL               5.0)
set_source_files_properties(shaders/ps_lines.hlsl PROPERTIES
    VS_SHADER_ENTRYPOINT          main_ps
    VS_SHADER_TYPE                Pixel
    VS_SHADER_OUTPUT_HEADER_FILE  shaders/ps_lines.h
    VS_SHADER_VARIABLE_NAME       k_ps_lines
    VS_SHADER_MODEL               5.0)
set_source_files_properties(shaders/vs_vertices_only.hlsl PROPERTIES
    VS_SHADER_ENTRYPOINT          main_vs
    VS_SHADER_TYPE                Vertex
    VS_SHADER_OUTPUT_HEADER_FILE  shaders/vs_vertices_only.h
    VS_SHADER_VARIABLE_NAME       k_vs_vertices_only
    VS_SHADER_MODEL               5.0)
set_source_files_properties(shaders/ps_vertices_only.hlsl PROPERTIES
    VS_SHADER_ENTRYPOINT          main_ps
    VS_SHADER_TYPE                Pixel
    VS_SHADER_OUTPUT_HEADER_FILE  shaders/ps_vertices_only.h
    VS_SHADER_VARIABLE_NAME       k_ps_vertices_only
    VS_SHADER_MODEL               5.0)
set_source_files_properties(shaders/vs_normals.hlsl PROPERTIES
    VS_SHADER_ENTRYPOINT          main_vs
    VS_SHADER_TYPE                Vertex
    VS_SHADER_OUTPUT_HEADER_FILE  shaders/vs_normals.h
    VS_SHADER_VARIABLE_NAME       k_vs_normals
    VS_SHADER_MODEL               5.0)
set_source_files_properties(shaders/ps_normals.hlsl PROPERTIES
    VS_SHADER_ENTRYPOINT          main_ps
    VS_SHADER_TYPE                Pixel
    VS_SHADER_OUTPUT_HEADER_FILE  shaders/ps_normals.h
    VS_SHADER_VARIABLE_NAME       k_ps_normals
    VS_SHADER_MODEL               5.0)

set(src_files
    render_main.cpp
    render_model.cpp
    render_lines.cpp
    shaders_database.cpp
    shaders_compiler.cpp
    render_vertices_only.cpp
    render_with_normals.cpp
    predefined_objects.cpp
    )
set(header_files
    stub_window.h
    utils.h
    render_model.h
    render_lines.h
    shaders_database.h
    shaders_compiler.h
    read_directory_changes.h
    io_completion_port.h
    render_vertices_only.h
    render_with_normals.h
    predefined_objects.h
    )
add_executable(${exe_name} WIN32 ${src_files} ${header_files} ${shaders_files})
target_compile_definitions(${exe_name} PRIVATE "XX_PACKAGE_FOLDER=\"${PROJECT_SOURCE_DIR}/assets/_package/\"")
target_compile_definitions(${exe_name} PRIVATE "XX_SHADERS_FOLDER=\"${PROJECT_SOURCE_DIR}/src/lr_app/shaders/\"")

target_compile_definitions(${exe_name} PRIVATE UNICODE _UNICODE)
if (${gcc_on_msvc})
    # Make GCC aware about wmain()
    set_target_properties(${exe_name} PROPERTIES LINK_FLAGS -municode)

    target_compile_options(${exe_name} PRIVATE
        -Wno-unknown-pragmas
        )
endif()

set_all_warnings(${exe_name} PRIVATE)

# Own dependencies.
target_link_libraries(${exe_name} lr_model)

# Generated shaders.
target_include_directories(${exe_name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Third-party.
target_link_libraries(${exe_name} DX11_Integrated)
target_link_libraries(${exe_name} ImGui_Integrated)
