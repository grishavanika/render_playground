set(exe_name lr_export)

# TODO: move to third_party folder.
find_package(assimp REQUIRED) # models loading
find_path(stb_image_INCLUDE_DIR NAMES stb_image.h REQUIRED) # textures loading

set(src_files
    export_main.cpp
    )
set(header_files
    )

add_executable(${exe_name} ${src_files} ${header_files})
set_all_warnings(${exe_name} PRIVATE)

# Own dependencies.
target_link_libraries(${exe_name} lr_model)

# Assimp.
message("Assimp: ${ASSIMP_INCLUDE_DIRS}")
target_include_directories(${exe_name} PRIVATE ${ASSIMP_INCLUDE_DIRS})
target_link_libraries(${exe_name} ${ASSIMP_LIBRARIES})

# stb_image.h.
message("stb_image.h: ${stb_image_INCLUDE_DIR}")
target_include_directories(${exe_name} PRIVATE ${stb_image_INCLUDE_DIR})
