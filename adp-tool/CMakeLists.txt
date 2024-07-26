cmake_minimum_required(VERSION 3.6)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
project(adp-tool VERSION 2.0)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/Adp.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/src/Adp.h")

# Determine architecture for macOS
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64") # Ensure both x86_64 and arm64 are specified
endif()

# Load dependencies start
if(WIN32)
    SET(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "Triplet" FORCE)
elseif(APPLE)
    SET(VCPKG_TARGET_TRIPLET "arm64-osx" CACHE STRING "Triplet" FORCE)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(EM_FLAGS " -lwebsocket.js -O3 -lhtml5.js -lhtml5 -sAUTO_NATIVE_LIBRARIES -pthread -sSHARED_MEMORY=1 -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=2 -s ALLOW_MEMORY_GROWTH -s ASYNCIFY=1 -s FULL_ES3=1 -s USE_WEBGL2=1 -s NO_DISABLE_EXCEPTION_CATCHING=1 -s DYNAMIC_EXECUTION=0 -s AUTO_JS_LIBRARIES=0 -s AUTO_NATIVE_LIBRARIES=0 --bind -s WASM=1 -s EXPORTED_RUNTIME_METHODS=['callMain']")
    add_compile_options(-g -pthread)
else()
    find_package(unofficial-nativefiledialog REQUIRED)
    find_package(libzippp REQUIRED)
endif()

add_subdirectory(lib)

find_package(fmt REQUIRED)
find_package(imgui REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Stb REQUIRED)
find_package(argparse REQUIRED)
# Load dependencies end

add_definitions(-D_WEBSOCKETPP_CPP11_FUNCTIONAL_)
add_definitions(-D_WEBSOCKETPP_CPP11_SYSTEM_ERROR_)
add_definitions(-D_WEBSOCKETPP_CPP11_RANDOM_DEVICE_)
add_definitions(-D_WEBSOCKETPP_CPP11_MEMORY_)
add_definitions(-D_WEBSOCKETPP_CPP11_TYPE_TRAITS_)
add_definitions(-D_WEBSOCKETPP_CPP11_THREAD_)

include_directories(
    "lib/avrdude"
    "lib/argparse"
    "src"
)

set(GLOBAL_LIBS
    hidapi
    fmt::fmt
    nlohmann_json::nlohmann_json
    esp_serial
)

if(NOT EMSCRIPTEN)
    list(APPEND GLOBAL_LIBS avrdude serial libzippp::libzippp)
endif()

if(EMSCRIPTEN)
    # No additional platform-specific libraries for Emscripten
elseif(WIN32 OR MINGW)
    list(APPEND GLOBAL_LIBS setupapi ws2_32)
elseif(UNIX AND NOT APPLE)
    # Linux-specific libraries
    list(APPEND GLOBAL_LIBS udev X11)
endif()

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
    list(APPEND SHARED_SOURCES "src/Model/FirmwareBack.cpp")
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

set(GUI_LIBS
    ${GLOBAL_LIBS}
    imgui::imgui
)

if(NOT EMSCRIPTEN)
    list(APPEND GUI_LIBS unofficial::nativefiledialog::nfd)
    if(APPLE)
        list(APPEND GUI_LIBS "-framework OpenGL")
    else()
        if(UNIX AND NOT APPLE)
            list(APPEND GUI_LIBS GL)
        else()
            list(APPEND GUI_LIBS opengl32)
        endif()
    endif()
endif()

add_executable(${PROJECT_NAME} ${GUI_SOURCES})
# No static library options for Clang on macOS
# if(MINGW)
#     target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
# endif()
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

if(NOT EMSCRIPTEN)
    # CLI application start
    file(GLOB_RECURSE CLI_SOURCES
        ${SHARED_SOURCES}
        "src/Cli.cpp")

    add_executable("adp-cli" ${CLI_SOURCES})
    # No static library options for Clang on macOS
    # target_link_options("adp-cli" PRIVATE -static-libgcc -static-libstdc++)
    set_target_properties("adp-cli" PROPERTIES
        CXX_STANDARD 20
        CXX_EXTENSIONS OFF)
    target_link_libraries("adp-cli" ${GLOBAL_LIBS} argparse::argparse)
    if(EMSCRIPTEN)
        set_target_properties("adp-cli" PROPERTIES LINK_FLAGS ${EM_FLAGS})
    else()
        target_compile_definitions("adp-cli" PRIVATE DEVICE_SERVER_ENABLED)
        target_compile_definitions("adp-cli" PRIVATE DEVICE_CLIENT_ENABLED)
    endif()
    # CLI application end
endif()

# Debian installation start
if(UNIX AND NOT APPLE)
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
