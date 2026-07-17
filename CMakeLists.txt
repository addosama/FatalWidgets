# CMakeList.txt: FatalWidgets 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.12)
set(CMAKE_CXX_STANDARD 20)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("FatalWidgets")

add_subdirectory(ImGui)
add_subdirectory(FatalUITest)

# 将源代码添加到此项目的可执行文件。
add_library (FatalWidgets STATIC)
target_link_libraries(
	FatalWidgets
	PUBLIC
	ImGui
)
target_include_directories(
	FatalWidgets
	PUBLIC
	"include"
)
target_sources (
	FatalWidgets
	PRIVATE
	"src/FatalWidgets.cpp"
)

# TODO: 如有需要，请添加测试并安装目标。
