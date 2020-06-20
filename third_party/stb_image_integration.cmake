# textures loading

add_library(stb_image_Integrated INTERFACE)

# cmake -G "Visual Studio 16 2019" ^
#   -DCMAKE_TOOLCHAIN_FILE=C:\libs\vcpkg\scripts\buildsystems\vcpkg.cmake ^
#   -A x64 ..
find_path(stb_image_INCLUDE_DIR NAMES stb_image.h)
if (NOT ${stb_image_INCLUDE_DIR} STREQUAL "stb_image_INCLUDE_DIR-NOTFOUND")
    message("stb_image.h: ${stb_image_INCLUDE_DIR}")
    target_include_directories(stb_image_Integrated INTERFACE ${stb_image_INCLUDE_DIR})
else ()
    set(stb_image_header
        "${CMAKE_CURRENT_SOURCE_DIR}/deps/stb_image-src/stb_image.h")
    set(stb_image_url
        "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h")
    if (NOT EXISTS ${stb_image_header})
        file(DOWNLOAD ${stb_image_url} ${stb_image_header})
    endif ()
    get_filename_component(dir ${stb_image_header} DIRECTORY)
    target_include_directories(stb_image_Integrated INTERFACE "${dir}")
endif ()
