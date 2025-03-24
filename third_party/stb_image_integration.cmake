# textures loading

add_library(stb_image_Integrated INTERFACE)
find_package(Stb REQUIRED)
target_include_directories(stb_image_Integrated INTERFACE ${Stb_INCLUDE_DIR})
