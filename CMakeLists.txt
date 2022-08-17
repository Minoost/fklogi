cmake_minimum_required(VERSION 3.22)
project(fklogi)

set(CMAKE_CXX_STANDARD 23)

find_program(NUGET_EXE NAMES nuget)
if(NOT NUGET_EXE)
	message("NUGET.EXE not found.")
	message(FATAL_ERROR "Please install this executable, and run CMake again.")
endif()

execute_process(COMMAND ${NUGET_EXE} install "Microsoft.Windows.CppWinRT" -Version 2.0.220608.4 -ExcludeVersion -OutputDirectory ${CMAKE_BINARY_DIR}/packages)

add_executable(fklogi main.cpp)

target_link_options(fklogi PRIVATE /SUBSYSTEM:windows)
target_compile_definitions(fklogi PUBLIC /ZW)
target_compile_options(fklogi PRIVATE /await /bigobj)
set_target_properties(fklogi PROPERTIES
	VS_WINRT_EXTENSIONS TRUE
	VS_PROJECT_IMPORT ${CMAKE_BINARY_DIR}/packages/Microsoft.Windows.CppWinRT/build/native/Microsoft.Windows.CppWinRT.props
)
