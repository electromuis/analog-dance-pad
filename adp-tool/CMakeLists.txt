cmake_minimum_required(VERSION 3.16)

project(adp-tool VERSION 2.2 LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/Adp.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/src/Adp.h")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
	set(ADP_LINUX ON)
endif()

option(ADP_STATIC "Link vcpkg dependencies and the C++ runtime statically" ON)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
if(ADP_STATIC)
	if(MINGW)
		set(VCPKG_TARGET_TRIPLET "x64-mingw-static" CACHE STRING "Triplet" FORCE)
	elseif(WIN32)
		set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "Triplet" FORCE)
	elseif(ADP_LINUX)
		# Static libs, dynamic glibc (the vcpkg default on Linux, made explicit)
		set(VCPKG_TARGET_TRIPLET "x64-linux" CACHE STRING "Triplet")
	endif()
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")

if(EMSCRIPTEN)
	set(CMAKE_EXECUTABLE_SUFFIX ".html")
	add_compile_options(-g -pthread)

	set(ADP_EM_LINK_OPTIONS
		-O3
		-pthread
		--bind
		-lwebsocket.js
		-lhtml5.js
		-lhtml5
		-sPTHREAD_POOL_SIZE=2
		-sALLOW_MEMORY_GROWTH=1
		-sASYNCIFY=1
		-sFULL_ES3=1
		-sUSE_WEBGL2=1
		-sUSE_GLFW=3
		-sNO_DISABLE_EXCEPTION_CATCHING=1
		-sDYNAMIC_EXECUTION=0
		-sAUTO_JS_LIBRARIES=0
		-sAUTO_NATIVE_LIBRARIES=0
		-sWASM=1
		-sEXPORTED_RUNTIME_METHODS=callMain
	)
else()
	find_package(unofficial-nativefiledialog REQUIRED)
	find_package(platform_folders REQUIRED)
	find_package(libzippp REQUIRED)
	find_package(OpenGL REQUIRED)
	if(ADP_LINUX)
		find_package(X11 REQUIRED)
	endif()
endif()

add_subdirectory(lib)

find_package(fmt REQUIRED)
find_package(imgui REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Stb REQUIRED)
find_package(argparse REQUIRED)

add_compile_definitions(
	_WEBSOCKETPP_CPP11_FUNCTIONAL_
	_WEBSOCKETPP_CPP11_SYSTEM_ERROR_
	_WEBSOCKETPP_CPP11_RANDOM_DEVICE_
	_WEBSOCKETPP_CPP11_MEMORY_
	_WEBSOCKETPP_CPP11_TYPE_TRAITS_
	_WEBSOCKETPP_CPP11_THREAD_
)

include_directories(
	"lib/avrdude"
	"lib/argparse"
	"src"
)

# ---------------------------------------------------------------------------
# Libraries
# ---------------------------------------------------------------------------
set(ADP_COMMON_LIBS
	hidapi
	fmt::fmt
	nlohmann_json::nlohmann_json
	esp_serial
)

if(NOT EMSCRIPTEN)
	list(APPEND ADP_COMMON_LIBS avrdude serial libzippp::libzippp)
endif()

if(WIN32)
	list(APPEND ADP_COMMON_LIBS setupapi mswsock ws2_32)
elseif(ADP_LINUX)
	list(APPEND ADP_COMMON_LIBS udev)
endif()

set(ADP_GUI_LIBS imgui::imgui)
if(NOT EMSCRIPTEN)
	list(APPEND ADP_GUI_LIBS
		unofficial::nativefiledialog::nfd
		OpenGL::GL
		sago::platform_folders
	)
	if(ADP_LINUX)
		list(APPEND ADP_GUI_LIBS X11::X11)
	endif()
endif()

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(ADP_SHARED_SOURCES
	"src/Model/Device.cpp"
	"src/Model/Firmware.cpp"
	"src/Model/Log.cpp"
	"src/Model/Reporter.cpp"
	"src/Model/Utils.cpp"
)

if(EMSCRIPTEN)
	list(APPEND ADP_SHARED_SOURCES "src/Model/DeviceClientEm.cpp")
else()
	list(APPEND ADP_SHARED_SOURCES
		"src/Model/FirmwareBack.cpp"
		"src/Model/DeviceServerWs.cpp"
		"src/Model/DeviceClientWspp.cpp"
	)
endif()

set(ADP_GUI_SOURCES
	${ADP_SHARED_SOURCES}
	"src/View/Application.cpp"
	"src/View/Colors.cpp"
	"src/View/DeviceTab.cpp"
	"src/View/Image.cpp"
	"src/View/LightsTab.cpp"
	"src/View/MappingTab.cpp"
	"src/View/SensitivityTab.cpp"
	"src/View/GraphTab.cpp"
	"src/View/UserConfig.cpp"

	"src/Main.cpp"
)

if(WIN32)
	list(APPEND ADP_GUI_SOURCES "src/Assets/Resource.rc")
endif()

set(ADP_CLI_SOURCES
	${ADP_SHARED_SOURCES}
	"src/Cli.cpp"
	"src/Gateway/GatewayServer.cpp"
)

# ---------------------------------------------------------------------------
# Common target setup
# ---------------------------------------------------------------------------
function(adp_setup_target target)
	target_link_libraries(${target} PRIVATE ${ARGN})
	target_compile_definitions(${target} PRIVATE DEVICE_CLIENT_ENABLED)

	if(EMSCRIPTEN)
		target_link_options(${target} PRIVATE ${ADP_EM_LINK_OPTIONS})
	else()
		target_compile_definitions(${target} PRIVATE DEVICE_SERVER_ENABLED)
	endif()

	if(ADP_STATIC)
		if(MINGW)
			# Fully static: no libgcc/libstdc++/libwinpthread DLLs next to the exe
			target_link_options(${target} PRIVATE -static)
		elseif(ADP_LINUX AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
			# Static C++ runtime (glibc, X11 and GL stay dynamic)
			target_link_options(${target} PRIVATE -static-libgcc -static-libstdc++)
		endif()
	endif()
endfunction()

# ---------------------------------------------------------------------------
# GUI application
# ---------------------------------------------------------------------------
add_executable(${PROJECT_NAME} ${ADP_GUI_SOURCES})
adp_setup_target(${PROJECT_NAME} ${ADP_COMMON_LIBS} ${ADP_GUI_LIBS})

if(WIN32)
	set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON)
	if(MSVC)
		target_link_options(${PROJECT_NAME} PRIVATE /ENTRY:mainCRTStartup)
	endif()
endif()

# ---------------------------------------------------------------------------
# CLI application
# ---------------------------------------------------------------------------
if(NOT EMSCRIPTEN)
	add_executable(adp-cli ${ADP_CLI_SOURCES})
	adp_setup_target(adp-cli ${ADP_COMMON_LIBS} argparse::argparse)
endif()

# ---------------------------------------------------------------------------
# Debian package
# ---------------------------------------------------------------------------
if(ADP_LINUX)
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/Assets/icon64.png"
		"${CMAKE_CURRENT_BINARY_DIR}/adp-tool.png" COPYONLY)
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/deb/adp-tool.desktop.in"
		"${CMAKE_CURRENT_BINARY_DIR}/adp-tool.desktop")

	install(TARGETS ${PROJECT_NAME} DESTINATION "/usr/bin")
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deb/99_adp.rules" DESTINATION "/etc/udev/rules.d")
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/adp-tool.png" DESTINATION "/usr/share/pixmaps")
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/adp-tool.desktop" DESTINATION "/usr/share/applications")

	set(CPACK_GENERATOR "DEB")
	set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Electromuis")
	set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
	set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
		"${CMAKE_CURRENT_SOURCE_DIR}/cmake/deb/postinst"
		"${CMAKE_CURRENT_SOURCE_DIR}/cmake/deb/postrm")

	include(CPack)
endif()