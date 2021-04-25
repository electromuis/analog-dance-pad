cmake_minimum_required (VERSION 3.6)

set(CMAKE_GENERATOR_PLATFORM win32)
project(adp-tool)

add_subdirectory (lib/wxwidgets)
set(wxBUILD_SHARED OFF)

add_executable (${PROJECT_NAME} WIN32
	src/Main.cpp
	src/Model/Device.cpp
	src/Model/Log.cpp
	src/Model/Reporter.cpp
	src/Model/Utils.cpp
	src/View/AboutTab.cpp
	src/View/DeviceTab.cpp
	src/View/IdleTab.cpp
	src/View/LightsTab.cpp
	src/View/LogTab.cpp
	src/View/MappingTab.cpp
	src/View/SensitivityTab.cpp
	src/View/Style.cpp
	
	lib/hidapi/windows/hid.c
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
	PUBLIC "lib/wxwidgets/include"
	PUBLIC "lib/hidapi/hidapi"
	PUBLIC "src"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_EXTENSIONS OFF
)

target_link_libraries(${PROJECT_NAME} 
	wx::net wx::core wx::base
	setupapi
)