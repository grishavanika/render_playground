set(exe_name learn_rendering)

find_package(DirectX11 REQUIRED)

target_collect_sources(${exe_name})

set(shaders_files
	shaders/vs.hlsl
	shaders/ps.hlsl)
set_source_files_properties(shaders/vs.hlsl PROPERTIES
	VS_SHADER_ENTRYPOINT main_vs
	VS_SHADER_TYPE Vertex
	VS_SHADER_OUTPUT_HEADER_FILE shaders/vs.h
	VS_SHADER_VARIABLE_NAME k_VS)
set_source_files_properties(shaders/ps.hlsl PROPERTIES
	VS_SHADER_ENTRYPOINT main_ps
	VS_SHADER_TYPE Pixel
	VS_SHADER_OUTPUT_HEADER_FILE shaders/ps.h
	VS_SHADER_VARIABLE_NAME k_PS)

add_executable(${exe_name} WIN32 ${${exe_name}_files} ${shaders_files})

target_compile_definitions(${exe_name} PRIVATE UNICODE _UNICODE)
if (${gcc_on_msvc})
	# Make GCC aware about wmain()
	set_target_properties(${exe_name} PROPERTIES LINK_FLAGS -municode)
endif()

set_all_warnings(${exe_name} PRIVATE)

target_include_directories(${exe_name} PUBLIC include)
# for shaders
target_include_directories(${exe_name} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
# DX11
target_include_directories(${exe_name} PUBLIC ${DirectX11_INCLUDE_DIRS})
# temporary: <xnamath.h>
target_include_directories(${exe_name} PUBLIC "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include")

target_link_libraries(${exe_name} ${DirectX11_LIBRARY})
