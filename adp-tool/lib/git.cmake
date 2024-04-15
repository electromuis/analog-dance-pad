message(STATUS "Finding git")

include("FindGit")

if(GIT_FOUND)
	if(EXISTS "${PROJECT_SOURCE_DIR}/../.git")
			
		message(STATUS "Submodule update")
		
		execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
							WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
							RESULT_VARIABLE GIT_SUBMOD_RESULT )
	
	else()
		message(STATUS "No git project")
	endif()
else()
	message(STATUS "No git found")
endif()
