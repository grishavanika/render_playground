add_library(Assimp_Integrated INTERFACE)

# cmake -G "Visual Studio 16 2019" -DCMAKE_TOOLCHAIN_FILE=C:\libs\vcpkg\scripts\buildsystems\vcpkg.cmake -A x64 ..
find_package(assimp QUIET) # models loading
if (${assimp_FOUND})
    message("Assimp: ${ASSIMP_INCLUDE_DIRS}")

    target_include_directories(Assimp_Integrated INTERFACE ${ASSIMP_INCLUDE_DIRS})
    target_link_libraries(Assimp_Integrated INTERFACE ${ASSIMP_LIBRARIES})
else ()
    FetchContent_Declare(
        assimp_content
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/assimp-src"
        FULLY_DISCONNECTED ON)
    FetchContent_GetProperties(assimp_content)
    if (NOT assimp_content_POPULATED)
        FetchContent_Populate(assimp_content)
    endif ()

    # Assimp unconditionally sets special BUILD_SHARED_LIBS variable
    # that affects other libs. Remember what the variable was before
    # Assimp and then change the value to original one.
    set(xx_assimp_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS} CACHE BOOL "" FORCE)

    # We don't need any exporters;
    set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
    # and importers;
    set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
    # only OBJ format.
    set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)

    add_subdirectory(${assimp_content_SOURCE_DIR} ${assimp_content_BINARY_DIR} EXCLUDE_FROM_ALL)
    set(BUILD_SHARED_LIBS ${xx_assimp_BUILD_SHARED_LIBS} CACHE BOOL "" FORCE)

    target_link_libraries(Assimp_Integrated INTERFACE assimp)

    if (MSVC)
        target_compile_options(assimp PUBLIC
            # The std::iterator class template is deprecated
            /wd4996
            )
    endif ()

    if (clang_on_msvc)
        target_compile_options(assimp PUBLIC
            -Wno-pragma-pack
            -Wno-unused-variable
            -Wno-unused-value
            -Wno-microsoft-enum-value
            -Wno-documentation
            -Wno-documentation-unknown-command
            -Wno-switch-enum
            -Wno-implicit-float-conversion
            )

        target_compile_options(zlibstatic PUBLIC
            -Wno-unused-parameter
            )
    endif ()

    set_target_properties(assimp PROPERTIES FOLDER third_party)
endif ()
