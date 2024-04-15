# Load dependencies start
# include(git.cmake)


include(hidapi.cmake)
if(NOT EMSCRIPTEN)
	add_subdirectory (avrdude)
	include(serial.cmake)
endif()

include(esp-flasher.cmake)