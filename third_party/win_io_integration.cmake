include(FetchContent)
include(CMakePrintHelpers)

FetchContent_Declare(
  win_io
  GIT_REPOSITORY https://github.com/grishavanika/win_io.git
  GIT_TAG        d0e067621798feace84b8f9f3e2e4935e0183c7b
)
# FetchContent_MakeAvailable can't be used
# since win_io is not FetchContent-friendly
# (add_subdirectory will add more then win_io library)
FetchContent_Populate(win_io)
cmake_print_variables(win_io_SOURCE_DIR)

add_library(win_io INTERFACE)
target_include_directories(win_io INTERFACE
    "${win_io_SOURCE_DIR}/src/win_io/include/win_io")

