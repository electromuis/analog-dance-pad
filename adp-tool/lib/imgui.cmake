cmake_minimum_required (VERSION 3.6)

if(NOT EMSCRIPTEN)
    #set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/GLFW")

    #option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
    #option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
    #option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
    #option(GLFW_INSTALL "Generate installation target" OFF)
    #option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)

    #add_subdirectory (${GLFW_DIR})
    #include_directories(${GLFW_DIR}/deps)
endif()


# ImGui
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui")

include_directories(
	${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

add_library(imgui
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp

    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
)

if(WIN32)
    target_link_libraries(imgui PUBLIC glfw gdi32 opengl32 imm32)
elseif(UNIX)
    target_link_libraries(imgui PUBLIC glfw GL)
elseif(EMSCRIPTEN)
    target_link_libraries(imgui PUBLIC GL)
endif()
