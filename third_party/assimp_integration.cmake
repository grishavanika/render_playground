add_library(Assimp_Integrated INTERFACE)

find_package(assimp CONFIG REQUIRED)
target_link_libraries(Assimp_Integrated INTERFACE assimp::assimp)
