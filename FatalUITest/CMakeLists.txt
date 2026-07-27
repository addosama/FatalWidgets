add_executable(FatalUITest)

target_include_directories(FatalUITest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
	FatalUITest
	PUBLIC
	FatalWidgets
	d3d12
	dxgi
)

target_sources(
	FatalUITest
	PRIVATE
	fatalui_test.cpp
	imgui_impl_win32.cpp
	imgui_impl_dx12.cpp
	"menu/Client.h"
 "menu/Menu.cpp")