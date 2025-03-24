add_library(outcome INTERFACE)
find_package(outcome CONFIG REQUIRED)
target_link_libraries(outcome INTERFACE outcome::hl)
