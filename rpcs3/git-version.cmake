set(GIT_VERSION_FILE "${SOURCE_DIR}/git-version.h")
set(GIT_VERSION "unknown")
set(GIT_BRANCH "unknown")
set(GIT_VERSION_UPDATE "1")

find_package(Git)
if(GIT_FOUND AND EXISTS "${SOURCE_DIR}/../.git/")
	execute_process(COMMAND ${GIT_EXECUTABLE} rev-list HEAD --count
		WORKING_DIRECTORY ${SOURCE_DIR}
		RESULT_VARIABLE exit_code
		OUTPUT_VARIABLE GIT_VERSION)
	if(NOT ${exit_code} EQUAL 0)
		message(WARNING "git rev-list failed, unable to include version.")
	endif()
	execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
		WORKING_DIRECTORY ${SOURCE_DIR}
		RESULT_VARIABLE exit_code
		OUTPUT_VARIABLE GIT_VERSION_)
	if(NOT ${exit_code} EQUAL 0)
		message(WARNING "git rev-parse failed, unable to include version.")
	endif()
	execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
		WORKING_DIRECTORY ${SOURCE_DIR}
		RESULT_VARIABLE exit_code
		OUTPUT_VARIABLE GIT_BRANCH)
	if(NOT ${exit_code} EQUAL 0)
		message(WARNING "git rev-parse failed, unable to include git branch.")
	endif()
	string(STRIP ${GIT_VERSION} GIT_VERSION)
	string(STRIP ${GIT_VERSION_} GIT_VERSION_)
	string(STRIP ${GIT_VERSION}-${GIT_VERSION_} GIT_VERSION)
	string(STRIP ${GIT_BRANCH} GIT_BRANCH)
	message(STATUS "GIT_VERSION: " ${GIT_VERSION})
	message(STATUS "GIT_BRANCH: " ${GIT_BRANCH})
else()
	message(WARNING "git not found, unable to include version.")
endif()

if(EXISTS ${GIT_VERSION_FILE})
	# Don't update if marked not to update.
	file(STRINGS ${GIT_VERSION_FILE} match
		REGEX "RPCS3_GIT_VERSION_NO_UPDATE 1")
	if(NOT ${match} EQUAL "")
		set(GIT_VERSION_UPDATE "0")
	endif()

	# Don't update if it's already the same.
	file(STRINGS ${GIT_VERSION_FILE} match
		REGEX "${GIT_VERSION}")
	if(NOT ${match} EQUAL "")
		set(GIT_VERSION_UPDATE "0")
	endif()	
endif()

set(code_string "// This is a generated file.\n\n"
	"#define RPCS3_GIT_VERSION \"${GIT_VERSION}\"\n"
	"#define RPCS3_GIT_BRANCH \"${GIT_BRANCH}\"\n\n"
	"// If you don't want this file to update/recompile, change to 1.\n"
	"#define RPCS3_GIT_VERSION_NO_UPDATE 0\n")

if ("${GIT_VERSION_UPDATE}" EQUAL "1")
	file(WRITE ${GIT_VERSION_FILE} ${code_string})
endif()
