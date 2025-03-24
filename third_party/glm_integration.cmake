add_library(glm_Interface INTERFACE)

find_package(glm CONFIG REQUIRED)

target_compile_definitions(glm_Interface
  INTERFACE GLM_ENABLE_EXPERIMENTAL)

target_link_libraries(glm_Interface INTERFACE glm::glm-header-only)
