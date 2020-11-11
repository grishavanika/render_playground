include(FetchContent)
include(CMakePrintHelpers)

set(win_io_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/deps/win_io-src")

FetchContent_Declare(
    win_io
    SOURCE_DIR "${win_io_SOURCE}"
    FULLY_DISCONNECTED ON)
FetchContent_GetProperties(win_io)
if (NOT win_io_POPULATED)
    FetchContent_Populate(win_io)
endif ()
cmake_print_variables(win_io_SOURCE_DIR)

add_library(win_io INTERFACE)
target_sources(win_io INTERFACE
    "${win_io_SOURCE}/src/win_io/include/win_io/detail/io_completion_port.h"
    "${win_io_SOURCE}/src/win_io/include/win_io/detail/read_directory_changes.h"
    )
target_include_directories(win_io INTERFACE
    "${win_io_SOURCE}/src/win_io/include/win_io/detail")

