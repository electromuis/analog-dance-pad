cmake_minimum_required (VERSION 3.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
project(adp-tool VERSION 2.0)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/Adp.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/src/Adp.h")

# Load dependencies start
# include(lib/git.cmake)

add_subdirectory (lib/fmt)
include(lib/imgui.cmake)
include(lib/hidapi.cmake)
include(lib/asio.cmake)
include_directories(
	"${CMAKE_CURRENT_SOURCE_DIR}/lib/hidapi/hidapi"
)

if(NOT EMSCRIPTEN)
	add_subdirectory (lib/avrdude)
	include(lib/nfd.cmake)
	include(lib/serial.cmake)
else()
	set(CMAKE_EXECUTABLE_SUFFIX ".html")
	set(EM_FLAGS "-g -s ALLOW_MEMORY_GROWTH -s DEMANGLE_SUPPORT=1 -s ASYNCIFY=1 -s FULL_ES3=1 -s USE_WEBGL2=1 -s USE_GLFW=3 -s NO_DISABLE_EXCEPTION_CATCHING=1 -s DYNAMIC_EXECUTION=0 -s AUTO_JS_LIBRARIES=0 --bind -s WASM=1 -s EXPORTED_RUNTIME_METHODS=['callMain']")
	add_compile_options(-g)
endif()

include_directories(
	"lib/stb_image"
	"lib/avrdude"
	"lib/argparse"
	"lib/json/single_include"
	"src")

set(GLOBAL_LIBS
	# asio
	hidapi
	fmt)

if(NOT EMSCRIPTEN)
	list(APPEND GLOBAL_LIBS avrdude serial)
endif()

if(EMSCRIPTEN)

elseif(WIN32)
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

#source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${GUI_SOURCES})
#set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

set(GUI_LIBS
	${GLOBAL_LIBS}
	imgui)

if(NOT EMSCRIPTEN)
	list(APPEND GUI_LIBS nfd)
endif()

# if(WIN32) list(APPEND sources "src/Assets/Resource.rc") endif()

add_executable (${PROJECT_NAME} ${GUI_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 20
	CXX_EXTENSIONS OFF)
target_link_libraries(${PROJECT_NAME} ${GUI_LIBS})
if(EMSCRIPTEN)
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS ${EM_FLAGS})
endif()
# GUI application end

# CLI application start
file(GLOB_RECURSE CLI_SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/lib/hidapi/wasm/hid.cpp"
     "src/*.h"
     "src/Model/*.cpp"
	 "src/Cli.cpp")

#source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${CLI_SOURCES})

add_executable ("adp-cli" ${CLI_SOURCES})
set_target_properties("adp-cli" PROPERTIES
	CXX_STANDARD 20
	CXX_EXTENSIONS OFF)
target_link_libraries("adp-cli" ${GLOBAL_LIBS})
if(EMSCRIPTEN)
	set_target_properties("adp-cli" PROPERTIES LINK_FLAGS ${EM_FLAGS})
endif()
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