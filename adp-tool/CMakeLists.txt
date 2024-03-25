cmake_minimum_required (VERSION 3.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
project(adp-tool VERSION 2.0)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/Adp.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/src/Adp.h")

add_subdirectory(lib)

find_package(fmt REQUIRED)
find_package(imgui REQUIRED)
find_package(libzippp REQUIRED)
find_package(unofficial-nativefiledialog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Stb REQUIRED)
find_package(argparse REQUIRED)

if(EMSCRIPTEN)
	set(CMAKE_EXECUTABLE_SUFFIX ".html")

	set(EM_FLAGS " -lwebsocket.js -O3 -s ALLOW_MEMORY_GROWTH -s ASYNCIFY=1 -s FULL_ES3=1 -s USE_WEBGL2=1 -s USE_GLFW=3 -s NO_DISABLE_EXCEPTION_CATCHING=1 -s DYNAMIC_EXECUTION=0 -s AUTO_JS_LIBRARIES=0 -s AUTO_NATIVE_LIBRARIES=0 --bind -s WASM=1 -s EXPORTED_RUNTIME_METHODS=['callMain']")
	 #set(EM_FLAGS "-lwebsocket.js -g -s ALLOW_MEMORY_GROWTH -s DEMANGLE_SUPPORT=1 -s ASYNCIFY=1 -s FULL_ES3=1 -s USE_WEBGL2=1 -s USE_GLFW=3 -s NO_DISABLE_EXCEPTION_CATCHING=1 -s DYNAMIC_EXECUTION=0 -s AUTO_JS_LIBRARIES=0 -s AUTO_NATIVE_LIBRARIES=0 --bind -s WASM=1 -s EXPORTED_RUNTIME_METHODS=['callMain']")
	# set(EM_FLAGS "-g -s ALLOW_MEMORY_GROWTH -s DEMANGLE_SUPPORT=1 -s ASYNCIFY=1 -s FULL_ES3=1 -s USE_WEBGL2=1 -s USE_GLFW=3 -s NO_DISABLE_EXCEPTION_CATCHING=1 -s DYNAMIC_EXECUTION=0 -s AUTO_JS_LIBRARIES=0 --bind -s WASM=1 -s EXPORTED_RUNTIME_METHODS=['callMain']")

	add_compile_options(-g)
endif()

add_definitions (-D_WEBSOCKETPP_CPP11_FUNCTIONAL_)
add_definitions (-D_WEBSOCKETPP_CPP11_SYSTEM_ERROR_)
add_definitions (-D_WEBSOCKETPP_CPP11_RANDOM_DEVICE_)
add_definitions (-D_WEBSOCKETPP_CPP11_MEMORY_)
add_definitions (-D_WEBSOCKETPP_CPP11_TYPE_TRAITS_)
add_definitions (-D_WEBSOCKETPP_CPP11_THREAD_)

# find_path(SYSTEM_INCLUDE_DIR zlib.h)

include_directories(
	"lib/avrdude"
	"lib/argparse"
	"src"
	# ${SYSTEM_INCLUDE_DIR}
)

set(GLOBAL_LIBS
	hidapi
	fmt::fmt
	libzippp::libzippp
	nlohmann_json::nlohmann_json
	esp_serial)

if(NOT EMSCRIPTEN)
	list(APPEND GLOBAL_LIBS avrdude serial)
endif()

if(EMSCRIPTEN)

elseif(WIN32 OR MINGW)
	list(APPEND GLOBAL_LIBS setupapi ws2_32)
elseif(UNIX)
	list(APPEND GLOBAL_LIBS udev X11)
endif()
# Load dependencies end

set(SHARED_SOURCES
	"src/Model/Device.cpp"
	"src/Model/Firmware.cpp"
	"src/Model/Log.cpp"
	"src/Model/Reporter.cpp"
	"src/Model/Utils.cpp"
)

if(EMSCRIPTEN)
	list(APPEND SHARED_SOURCES "src/Model/DeviceClientEm.cpp")
else()
	list(APPEND SHARED_SOURCES "src/Model/DeviceServerWs.cpp")
	list(APPEND SHARED_SOURCES "src/Model/DeviceClientWspp.cpp")
endif()

# GUI application start

set(GUI_SOURCES
	${SHARED_SOURCES}

	"src/View/Application.cpp"
	"src/View/Colors.cpp"
	"src/View/DeviceTab.cpp"
	"src/View/Image.cpp"
	"src/View/LightsTab.cpp"
	"src/View/MappingTab.cpp"
	"src/View/SensitivityTab.cpp"

	"src/Main.cpp"
)

#source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${GUI_SOURCES})
#set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

set(GUI_LIBS
	${GLOBAL_LIBS}
	opengl32
	imgui::imgui
	unofficial::nativefiledialog::nfd)

# if(NOT EMSCRIPTEN)
# 	list(APPEND GUI_LIBS)
# endif()

# if(WIN32) list(APPEND sources "src/Assets/Resource.rc") endif()

add_executable (${PROJECT_NAME} ${GUI_SOURCES})
if(MINGW)
	target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 20
	CXX_EXTENSIONS OFF)
target_link_libraries(${PROJECT_NAME} ${GUI_LIBS})
target_compile_definitions(${PROJECT_NAME} PRIVATE DEVICE_CLIENT_ENABLED)

if(EMSCRIPTEN)
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS ${EM_FLAGS})
else()
	target_compile_definitions(${PROJECT_NAME} PRIVATE DEVICE_SERVER_ENABLED)
endif()
# GUI application end

# CLI application start
file(GLOB_RECURSE CLI_SOURCES
	${SHARED_SOURCES}
	"src/Cli.cpp")

#source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${CLI_SOURCES})

add_executable ("adp-cli" ${CLI_SOURCES})
target_link_options("adp-cli" PRIVATE -static-libgcc -static-libstdc++)
set_target_properties("adp-cli" PROPERTIES
	CXX_STANDARD 20
	CXX_EXTENSIONS OFF)
target_link_libraries("adp-cli" ${GLOBAL_LIBS} argparse::argparse)
if(EMSCRIPTEN)
	set_target_properties("adp-cli" PROPERTIES LINK_FLAGS ${EM_FLAGS})
else()
	target_compile_definitions(${PROJECT_NAME} PRIVATE DEVICE_SERVER_ENABLED)
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