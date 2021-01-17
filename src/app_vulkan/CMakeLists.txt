set(exe_name app_vulkan)

set(src_files
    main.cpp
    )
set(header_files
    stub_window.h
    utils.h
    )

add_executable(${exe_name} WIN32 ${src_files} ${header_files} ${shaders_files})
target_link_libraries(${exe_name} PRIVATE Vulkan_Integrated)
target_compile_definitions(${exe_name} PRIVATE UNICODE _UNICODE)

if (${gcc_on_msvc})
    # Make GCC aware about wmain()
    set_target_properties(${exe_name} PROPERTIES LINK_FLAGS -municode)

    target_compile_options(${exe_name} PRIVATE
        -Wno-unknown-pragmas
        )
endif()

set_all_warnings(${exe_name} PRIVATE)
