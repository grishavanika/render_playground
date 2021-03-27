list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../tools/cmake")
find_package(DirectX11 REQUIRED)
message("DirectX11: ${DirectX11_LIBRARY}")

add_library(Vulkan_Integrated INTERFACE)

# TODO: temporary hard-code paths. Nice find_package()
# or something similar should be there.
target_include_directories(Vulkan_Integrated INTERFACE
    "C:/Programs/VulkanSDK/1.2.170.0/Include")
target_link_libraries(Vulkan_Integrated INTERFACE
    "C:/Programs/VulkanSDK/1.2.170.0/Lib/vulkan-1.lib")
