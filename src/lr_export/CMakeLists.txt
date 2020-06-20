set(exe_name lr_export)

set(src_files
    export_main.cpp
    )
set(header_files
    )

add_executable(${exe_name} ${src_files} ${header_files})
set_all_warnings(${exe_name} PRIVATE)

# Own dependencies.
target_link_libraries(${exe_name} lr_model)

# Third-party.
target_link_libraries(${exe_name} Assimp_Integrated)
target_link_libraries(${exe_name} stb_image_Integrated)
