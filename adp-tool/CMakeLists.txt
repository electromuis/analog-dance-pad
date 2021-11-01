cmake_minimum_required (VERSION 3.6)


	set(CMAKE_GENERATOR_PLATFORM win32)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
project(adp-tool)

set( INCLUDES
             INTERFACE
                ${wxWidgets_INCLUDE_DIRS}
          )

set(wxBUILD_SHARED OFF CACHE BOOL "enable X functionality")
add_subdirectory (lib/wxwidgets)
add_subdirectory (lib/avrdude)
include(lib/hidapi.cmake)
include(lib/serial.cmake)

file(GLOB_RECURSE sources
     "src/*.h"
     "src/*.cpp"
)

if(WIN32)
	list(APPEND sources "src/Assets/Resource.rc")
endif()

add_executable (${PROJECT_NAME} WIN32 ${sources})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${sources})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

if(WIN32)
	set(WX_SETUP_INCLUDE "${CMAKE_CURRENT_BINARY_DIR}/lib/vc_lib/mswud")
else()
	set(WX_SETUP_INCLUDE "${CMAKE_CURRENT_BINARY_DIR}/lib/wx/include/gtk3-unicode-static-3.1")
endif()

target_include_directories(${PROJECT_NAME}
	PUBLIC "${WX_SETUP_INCLUDE}"
	PUBLIC "lib/wxwidgets/include"
	PUBLIC "lib/avrdude"
	PUBLIC "lib/json/single_include"
	PUBLIC "src"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_EXTENSIONS OFF
)

set(LIBRARIES
	wx::net wx::core wx::base
	hidapi
	avrdude
	serial
)

if(WIN32)
	list(APPEND LIBRARIES setupapi)
elseif(UNIX)
	list(APPEND LIBRARIES udev X11)
endif()

target_link_libraries(${PROJECT_NAME} ${LIBRARIES})
