cmake_minimum_required (VERSION 3.6)

set(HIDAPI_PATH "${CMAKE_CURRENT_LIST_DIR}/hidapi")

if(EMSCRIPTEN)
    list(APPEND HIDAPI_SRC ${HIDAPI_PATH}/wasm/hid.cpp)
elseif(APPLE)
    list(APPEND HIDAPI_SRC ${HIDAPI_PATH}/mac/hid.c)
elseif(UNIX)
    list(APPEND HIDAPI_SRC ${HIDAPI_PATH}/linux/hid.c)
elseif(WIN32)
    list(APPEND HIDAPI_SRC ${HIDAPI_PATH}/windows/hid.c)
endif()

add_library(hidapi ${HIDAPI_SRC})
target_include_directories(hidapi PUBLIC ${HIDAPI_PATH}/hidapi)