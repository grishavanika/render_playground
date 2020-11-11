add_library(glm_Interface INTERFACE)

FetchContent_Declare(
    glm_content
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/glm-src"
    FULLY_DISCONNECTED ON)
FetchContent_GetProperties(glm_content)
if (NOT glm_content_POPULATED)
    FetchContent_Populate(glm_content)
endif ()

add_subdirectory(${glm_content_SOURCE_DIR} ${glm_content_BINARY_DIR} EXCLUDE_FROM_ALL)
target_link_libraries(glm_Interface INTERFACE glm)
