cmake_minimum_required (VERSION 3.6)

set(ASIO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/asio")

include_directories(
	${ASIO_PATH}/include
)

list(APPEND ASIO_SRC ${ASIO_PATH}/src/asio.cpp)

# add_library(asio ${ASIO_SRC})