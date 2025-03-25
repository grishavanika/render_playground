set(exe_name app)

set(shaders_files)

macro(add_vs_shader file)
    list(APPEND shaders_files ${PROJECT_SOURCE_DIR}/shaders/${file}.hlsl)
    set_source_files_properties(${PROJECT_SOURCE_DIR}/shaders/${file}.hlsl PROPERTIES
        VS_SHADER_ENTRYPOINT          main_vs
        VS_SHADER_TYPE                Vertex
        VS_SHADER_OUTPUT_HEADER_FILE  shaders/${file}.h
        VS_SHADER_VARIABLE_NAME       k_${file}
        VS_SHADER_MODEL               5.0)
endmacro()

macro(add_ps_shader file)
    list(APPEND shaders_files ${PROJECT_SOURCE_DIR}/shaders/${file}.hlsl)
    set_source_files_properties(${PROJECT_SOURCE_DIR}/shaders/${file}.hlsl PROPERTIES
        VS_SHADER_ENTRYPOINT          main_ps
        VS_SHADER_TYPE                Pixel
        VS_SHADER_OUTPUT_HEADER_FILE  shaders/${file}.h
        VS_SHADER_VARIABLE_NAME       k_${file}
        VS_SHADER_MODEL               5.0)
endmacro()

macro(add_shader_as_header file)
    list(APPEND shaders_files ${PROJECT_SOURCE_DIR}/shaders/${file}.hlsl)
    set_source_files_properties(${PROJECT_SOURCE_DIR}/shaders/${file}.hlsl PROPERTIES
        HEADER_FILE_ONLY TRUE)
endmacro()

add_vs_shader(vs_basic_phong_lighting)
add_vs_shader(vs_gooch_shading)
add_vs_shader(vs_lines)
add_vs_shader(vs_vertices_only)
add_vs_shader(vs_normals)

add_ps_shader(ps_basic_phong_lighting)
add_ps_shader(ps_gooch_shading)
add_ps_shader(ps_lines)
add_ps_shader(ps_vertices_only)
add_ps_shader(ps_normals)

add_shader_as_header(common_basic_phong_lighting)
add_shader_as_header(common_gooch_shading)
add_shader_as_header(common_lines)

set(src_files
    main.cpp
    render_model.cpp
    render_lines.cpp
    shaders_database.cpp
    shaders_compiler.cpp
    render_vertices_only.cpp
    render_with_normals.cpp
    predefined_objects.cpp
    imgui_state_debug.cpp
    app_state.cpp
    assimp_model.cpp
    model.cpp
    )
set(header_files
    stub_window.h
    utils.h
    render_model.h
    render_lines.h
    shaders_database.h
    shaders_compiler.h
    render_vertices_only.h
    render_with_normals.h
    predefined_objects.h
    dx_api.h
    imgui_state_debug.h
    app_state.h
    model.h
    vertex.h
    utils_outcome.h
    assimp_model.h
    )

add_executable(${exe_name} WIN32 ${src_files} ${header_files} ${shaders_files})
target_compile_definitions(${exe_name} PRIVATE "XX_PACKAGE_FOLDER=\"${PROJECT_SOURCE_DIR}/assets/\"")
target_compile_definitions(${exe_name} PRIVATE "XX_SHADERS_FOLDER=\"${PROJECT_SOURCE_DIR}/shaders/\"")

target_compile_definitions(${exe_name} PRIVATE UNICODE _UNICODE)
if (${gcc_on_msvc})
    # Make GCC aware about wmain()
    set_target_properties(${exe_name} PROPERTIES LINK_FLAGS -municode)

    target_compile_options(${exe_name} PRIVATE
        -Wno-unknown-pragmas
        )
endif()

set_all_warnings(${exe_name} PRIVATE)

# Generated shaders.
target_include_directories(${exe_name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Third-party.
target_link_libraries(${exe_name} DX11_Integrated)
target_link_libraries(${exe_name} ImGui_Integrated)
target_link_libraries(${exe_name} win_io)
target_link_libraries(${exe_name} Assimp_Integrated)
target_link_libraries(${exe_name} stb_image_Integrated)
target_link_libraries(${exe_name} outcome)
target_link_libraries(${exe_name} glm_Interface)
