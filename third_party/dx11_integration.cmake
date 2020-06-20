list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../tools/cmake")
find_package(DirectX11 REQUIRED)
message("DirectX11: ${DirectX11_LIBRARY}")

add_library(DX11_Integrated INTERFACE)
target_include_directories(DX11_Integrated INTERFACE ${DirectX11_INCLUDE_DIRS})
target_link_libraries(DX11_Integrated INTERFACE ${DirectX11_LIBRARY})
