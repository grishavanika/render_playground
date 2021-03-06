# ImGui integration

include(FetchContent)

FetchContent_Declare(
	imgui
	SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui-src"
	FULLY_DISCONNECTED ON)
FetchContent_GetProperties(imgui)
if (NOT imgui_POPULATED)
	FetchContent_Populate(imgui)
endif ()

add_library(ImGui_Core
	${imgui_SOURCE_DIR}/imconfig.h
	${imgui_SOURCE_DIR}/imgui.h
	${imgui_SOURCE_DIR}/imgui_internal.h
	${imgui_SOURCE_DIR}/imgui.cpp
	${imgui_SOURCE_DIR}/imgui_demo.cpp
	${imgui_SOURCE_DIR}/imgui_draw.cpp
	${imgui_SOURCE_DIR}/imgui_widgets.cpp)

target_include_directories(ImGui_Core PUBLIC ${imgui_SOURCE_DIR})

add_library(ImGui_Cpp
	${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.h
	${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp)

target_include_directories(ImGui_Cpp PUBLIC ${imgui_SOURCE_DIR}/misc/cpp)
target_link_libraries(ImGui_Cpp PRIVATE ImGui_Core)

# sdl/opengl3/GL3W
add_library(ImGui_Impl
	${imgui_SOURCE_DIR}/examples/imgui_impl_dx11.cpp
	${imgui_SOURCE_DIR}/examples/imgui_impl_dx11.h
	${imgui_SOURCE_DIR}/examples/imgui_impl_win32.cpp
	${imgui_SOURCE_DIR}/examples/imgui_impl_win32.h
	)

target_include_directories(ImGui_Impl PUBLIC ${imgui_SOURCE_DIR}/examples)
target_link_libraries(ImGui_Impl PRIVATE ImGui_Core)

add_library(ImGui_Integrated INTERFACE)
target_link_libraries(ImGui_Integrated INTERFACE
	ImGui_Core
	ImGui_Impl
	ImGui_Cpp)

set_target_properties(ImGui_Core PROPERTIES FOLDER third_party)
set_target_properties(ImGui_Impl PROPERTIES FOLDER third_party)
set_target_properties(ImGui_Cpp PROPERTIES FOLDER third_party)

