include(FetchContent)
include(CMakePrintHelpers)

set(outcome_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/deps/outcome-src")

FetchContent_Declare(
    outcome
    SOURCE_DIR "${outcome_SOURCE}"
    FULLY_DISCONNECTED ON)
FetchContent_GetProperties(outcome)
if (NOT outcome_POPULATED)
    FetchContent_Populate(outcome)
endif ()
cmake_print_variables(outcome_SOURCE_DIR)

add_library(outcome INTERFACE)
target_sources(outcome INTERFACE
    "${outcome_SOURCE}/single-header/outcome.hpp")
target_include_directories(outcome INTERFACE
    "${outcome_SOURCE}/single-header")
