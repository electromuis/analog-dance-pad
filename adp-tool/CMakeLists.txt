cmake_minimum_required (VERSION 3.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
project(adp-tool VERSION 2.0)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/Adp.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/src/Adp.h")

# Load dependencies start
include(lib/git.cmake)

add_subdirectory (lib/avrdude)
add_subdirectory (lib/fmt)
include(lib/imgui.cmake)
include(lib/nfd.cmake)
include(lib/hidapi.cmake)
include(lib/serial.cmake)

include_directories(
	"lib/stb_image"
	"lib/avrdude"
	"lib/argparse"
	"lib/json/single_include"
	"src")

set(GLOBAL_LIBS
	hidapi
	avrdude
	serial
	fmt)

if(WIN32)
	list(APPEND GLOBAL_LIBS setupapi)
elseif(UNIX)
	list(APPEND GLOBAL_LIBS udev X11)
endif()
# Load dependencies end

# GUI application start
file(GLOB_RECURSE GUI_SOURCES
     "src/*.h"
     "src/Assets/*.cpp"
     "src/Model/*.cpp"
     "src/View/*.cpp"
	 "src/Main.cpp")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${GUI_SOURCES})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

set(GUI_LIBS
	${GLOBAL_LIBS}
	imgui
	glfw
	nfd)

# if(WIN32) list(APPEND sources "src/Assets/Resource.rc") endif()

add_executable (${PROJECT_NAME} ${GUI_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 20
	CXX_EXTENSIONS OFF)
target_link_libraries(${PROJECT_NAME} ${GUI_LIBS})
# GUI application end

# CLI application start
file(GLOB_RECURSE CLI_SOURCES
     "src/*.h"
     "src/Model/*.cpp"
	 "src/Cli.cpp")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${CLI_SOURCES})

add_executable ("adp-cli" ${CLI_SOURCES})
set_target_properties("adp-cli" PROPERTIES
	CXX_STANDARD 20
	CXX_EXTENSIONS OFF)
target_link_libraries("adp-cli" ${GLOBAL_LIBS})
# CLI application end




# Debian installation start
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
# Debian installation end