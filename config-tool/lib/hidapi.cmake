cmake_minimum_required (VERSION 3.6)

set(CMAKE_GENERATOR_PLATFORM win32)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/lib/hidapi/hidapi
)

add_library(
	hidapi
	${CMAKE_CURRENT_SOURCE_DIR}/lib/hidapi/windows/hid.c
)