cmake_minimum_required (VERSION 3.6)

if(WIN32)
	set(CMAKE_GENERATOR_PLATFORM win32)
endif()

set(HIDAPI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/hidapi")

include_directories(
	${HIDAPI_PATH}/hidapi
)

if(APPLE)
    list(APPEND HIDAPI_SRC ${HIDAPI_PATH}/mac/hid.c)
elseif(UNIX)
    list(APPEND HIDAPI_SRC ${HIDAPI_PATH}/linux/hid.c)
elseif(WIN32)
    list(APPEND HIDAPI_SRC ${HIDAPI_PATH}/windows/hid.c)
endif()

add_library(hidapi ${HIDAPI_SRC})