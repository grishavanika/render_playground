set(exe_name export)

set(src_files
    main.cpp
    )
set(header_files
    )

add_executable(${exe_name} ${src_files} ${header_files})
target_compile_definitions(${exe_name} PRIVATE "XX_ASSETS_FOLDER=\"${PROJECT_SOURCE_DIR}/assets/\"")

set_all_warnings(${exe_name} PRIVATE)

# Own dependencies.
target_link_libraries(${exe_name} model)

# Third-party.
target_link_libraries(${exe_name} Assimp_Integrated)
target_link_libraries(${exe_name} stb_image_Integrated)
