include(FetchContent)
include(CMakePrintHelpers)

set(scope_guard_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/deps/scope_guard-src")

FetchContent_Declare(
    scope_guard
    SOURCE_DIR "${scope_guard_SOURCE}"
    FULLY_DISCONNECTED ON)
FetchContent_GetProperties(scope_guard)
if (NOT scope_guard_POPULATED)
    FetchContent_Populate(scope_guard)
endif ()
cmake_print_variables(scope_guard_SOURCE_DIR)

add_library(scope_guard INTERFACE)
target_sources(scope_guard INTERFACE
    "${scope_guard_SOURCE}/ScopeGuard.h")
target_include_directories(scope_guard INTERFACE
    "${scope_guard_SOURCE}")


