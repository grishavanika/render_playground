add_library(glm_Interface INTERFACE)

FetchContent_Declare(
    glm_content
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/glm-src"
    FULLY_DISCONNECTED ON)
FetchContent_GetProperties(glm_content)
if (NOT glm_content_POPULATED)
    FetchContent_Populate(glm_content)
endif ()

if (MSVC)
    target_compile_options(glm_Interface INTERFACE
        # nonstandard extension used: nameless struct/union
        /wd4201
        )
endif ()

add_subdirectory(${glm_content_SOURCE_DIR} ${glm_content_BINARY_DIR} EXCLUDE_FROM_ALL)
target_link_libraries(glm_Interface INTERFACE glm)
