set(lib_name lr_model)

set(src_files
    model.cpp
    )
set(header_files
    model.h
    vertex.h
    utils_outcome.h
    )

add_library(${lib_name} ${src_files} ${header_files})
set_all_warnings(${lib_name} PRIVATE)
target_include_directories(${lib_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${lib_name} PUBLIC outcome)
target_link_libraries(${lib_name} PUBLIC scope_guard)
