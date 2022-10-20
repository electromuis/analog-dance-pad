cmake_minimum_required (VERSION 3.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
project(adp-tool VERSION 1.3.0)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/Adp.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/src/Adp.h")

include(lib/git.cmake)

add_subdirectory (lib/avrdude)
add_subdirectory (lib/fmt)
include(lib/imgui.cmake)
include(lib/nfd.cmake)
include(lib/hidapi.cmake)
include(lib/serial.cmake)

file(GLOB_RECURSE sources
     "src/*.h"
     "src/*.cpp"
)

if(WIN32)
	# list(APPEND sources "src/Assets/Resource.rc")
endif()

add_executable (${PROJECT_NAME} ${sources})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${sources})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
	PUBLIC "lib/stb_image"
	PUBLIC "lib/avrdude"
	PUBLIC "lib/json/single_include"
	PUBLIC "src"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 20
	CXX_EXTENSIONS OFF
)

set(LIBRARIES
	hidapi
	avrdude
	serial
	imgui
	glfw
	nfd
	Vulkan::Vulkan
	fmt
)

if(WIN32)
	list(APPEND LIBRARIES setupapi)
elseif(UNIX)
	list(APPEND LIBRARIES udev X11)
endif()

target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

if(UNIX)	
	install(
	    TARGETS ${PROJECT_NAME}
	    DESTINATION "/usr/bin/"
	)

	install(
	    FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deb/99_adp.rules"
	    DESTINATION "/etc/udev/rules.d/"
	)
	
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/Assets/icon64.png" "${CMAKE_CURRENT_BINARY_DIR}/adp-tool.png" COPYONLY)
	install(
	    FILES "${CMAKE_CURRENT_BINARY_DIR}/adp-tool.png"
	    DESTINATION "/usr/share/pixmaps/"
	)
	
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/deb/adp-tool.desktop.in" "${CMAKE_CURRENT_BINARY_DIR}/adp-tool.desktop")
	install(
	    FILES "${CMAKE_CURRENT_BINARY_DIR}/adp-tool.desktop"
	    DESTINATION "/usr/share/applications/"
	)

	set(CPACK_GENERATOR "DEB")
	set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Electromuis")
	set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
	set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deb/postinst;${CMAKE_CURRENT_SOURCE_DIR}/cmake/deb/postrm")

	include(CPack)
endif()