cmake_minimum_required (VERSION 3.6)

set(CMAKE_GENERATOR_PLATFORM win32)
project(adp-tool)

set(wxBUILD_SHARED OFF CACHE BOOL "enable X functionality")
add_subdirectory (lib/wxwidgets)
include(lib/hidapi.cmake)

file(GLOB_RECURSE sources
     "src/*.h"
     "src/*.cpp"
)

list(APPEND ${sources} lib/hidapi/windows/hid.c)

add_executable (${PROJECT_NAME} WIN32
	${sources}
	
#	src/Main.cpp
#	src/Model/Device.cpp
#	src/Model/Log.cpp
#	src/Model/Reporter.cpp
#	src/Model/Utils.cpp
#	src/View/AboutTab.cpp
#	src/View/DeviceTab.cpp
#	src/View/IdleTab.cpp
#	src/View/LightsTab.cpp
#	src/View/LogTab.cpp
#	src/View/MappingTab.cpp
#	src/View/SensitivityTab.cpp
	
#	src/Assets/Assets.cpp
	
#	lib/hidapi/windows/hid.c
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${sources})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
	PUBLIC "lib/wxwidgets/include"
	PUBLIC "src"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_EXTENSIONS OFF
)

target_link_libraries(${PROJECT_NAME} 
	wx::net wx::core wx::base
	setupapi
	hidapi
)