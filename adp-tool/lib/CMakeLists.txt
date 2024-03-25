# Load dependencies start
# include(git.cmake)

if(NOT EMSCRIPTEN)
	add_subdirectory (avrdude)
	include(serial.cmake)
endif()

include(esp-flasher.cmake)
include(hidapi.cmake)