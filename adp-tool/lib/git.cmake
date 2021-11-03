message(STATUS "Finding git")

include("FindGit")


if(GIT_FOUND)
	if(EXISTS "${PROJECT_SOURCE_DIR}/../.git")
		if(WIN32)
			
			message(STATUS "Submodule update")
			
			execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
								WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
								RESULT_VARIABLE GIT_SUBMOD_RESULT )
		elseif(UNIX)
			message(STATUS "Submodule update")
			
			execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init lib/hidapi
								WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
								RESULT_VARIABLE GIT_SUBMOD_RESULT)
			execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init lib/json
								WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
								RESULT_VARIABLE GIT_SUBMOD_RESULT)
			execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init lib/serial
								WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
								RESULT_VARIABLE GIT_SUBMOD_RESULT)
		endif()
	else()
		message(STATUS "No git project")
	endif()
else()
	message(STATUS "No git found")
endif()
