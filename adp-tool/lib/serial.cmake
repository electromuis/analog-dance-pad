cmake_minimum_required (VERSION 3.6)

set(SERIAL_PATH "${CMAKE_CURRENT_LIST_DIR}/serial")

set(SERIAL_SOURCE
    ${SERIAL_PATH}/src/serial.cc
    ${SERIAL_PATH}/include/serial/serial.h
    ${SERIAL_PATH}/include/serial/v8stdint.h
)

if(APPLE)
    # If OSX
    list(APPEND SERIAL_SOURCE ${SERIAL_PATH}/src/impl/unix.cc)
    list(APPEND SERIAL_SOURCE ${SERIAL_PATH}/src/impl/list_ports/list_ports_osx.cc)
elseif(UNIX)
    # If unix
    list(APPEND SERIAL_SOURCE ${SERIAL_PATH}/src/impl/unix.cc)
    list(APPEND SERIAL_SOURCE ${SERIAL_PATH}/src/impl/list_ports/list_ports_linux.cc)
else()
    # If windows
    list(APPEND SERIAL_SOURCE ${SERIAL_PATH}/src/impl/win.cc)
    list(APPEND SERIAL_SOURCE ${SERIAL_PATH}/src/impl/list_ports/list_ports_win.cc)
endif()

add_library(serial ${SERIAL_SOURCE})

target_include_directories(serial PUBLIC
	${SERIAL_PATH}/include
)