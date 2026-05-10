set(RPCS3_GIT_VERSION "local_build")
set(RPCS3_GIT_BRANCH "local_build")
set(RPCS3_GIT_FULL_BRANCH "local_build")

find_package(Git)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git/")
	execute_process(COMMAND ${GIT_EXECUTABLE} rev-list HEAD --count
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		RESULT_VARIABLE exit_code
		OUTPUT_VARIABLE RPCS3_GIT_VERSION)
	if(NOT ${exit_code} EQUAL 0)
		message(WARNING "git rev-list failed, unable to include version.")
	endif()
	execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		RESULT_VARIABLE exit_code
		OUTPUT_VARIABLE GIT_VERSION_)
	if(NOT ${exit_code} EQUAL 0)
		message(WARNING "git rev-parse failed, unable to include version.")
	endif()
	execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		RESULT_VARIABLE exit_code
		OUTPUT_VARIABLE RPCS3_GIT_BRANCH)
	if(NOT ${exit_code} EQUAL 0)
		message(WARNING "git rev-parse failed, unable to include git branch.")
	endif()

	string(STRIP ${RPCS3_GIT_VERSION} RPCS3_GIT_VERSION)
	string(STRIP ${GIT_VERSION_} GIT_VERSION_)
	string(STRIP ${RPCS3_GIT_VERSION}-${GIT_VERSION_} RPCS3_GIT_VERSION)
	string(STRIP ${RPCS3_GIT_BRANCH} RPCS3_GIT_BRANCH)
else()
	message(WARNING "git not found, unable to include version.")
endif()

function(gen_git_version rpcs3_src_dir)
	set(GIT_VERSION_FILE "${rpcs3_src_dir}/git-version.h")
	set(GIT_VERSION_UPDATE "1")

	# These environment variables are defined by CI
	# BUILD_REPOSITORY_NAME will look like "RPCS3/rpcs3"
	# BUILD_SOURCEBRANCHNAME will look like "master"
	if (DEFINED ENV{BUILD_REPOSITORY_NAME} AND NOT "$ENV{BUILD_REPOSITORY_NAME}" STREQUAL "")
		set(RPCS3_GIT_FULL_BRANCH "$ENV{BUILD_REPOSITORY_NAME}/$ENV{BUILD_SOURCEBRANCHNAME}")
	endif()

	message(STATUS "RPCS3_GIT_VERSION: " ${RPCS3_GIT_VERSION})
	message(STATUS "RPCS3_GIT_BRANCH: " ${RPCS3_GIT_BRANCH})
	message(STATUS "RPCS3_GIT_FULL_BRANCH: " ${RPCS3_GIT_FULL_BRANCH})

	if(EXISTS ${GIT_VERSION_FILE})
		# Don't update if marked not to update.
		file(STRINGS ${GIT_VERSION_FILE} match
			REGEX "RPCS3_GIT_VERSION_NO_UPDATE 1")
		if(NOT "${match}" STREQUAL "")
			set(GIT_VERSION_UPDATE "0")
		endif()

		# Don't update if it's already the same.
		file(STRINGS ${GIT_VERSION_FILE} match
			REGEX "${RPCS3_GIT_VERSION}")
		if(NOT "${match}" STREQUAL "")
			set(GIT_VERSION_UPDATE "0")
		endif()
	endif()

	set(code_string "// This is a generated file.\n\n"
		"#define RPCS3_GIT_VERSION \"${RPCS3_GIT_VERSION}\"\n"
		"#define RPCS3_GIT_BRANCH \"${RPCS3_GIT_BRANCH}\"\n"
		"#define RPCS3_GIT_FULL_BRANCH \"${RPCS3_GIT_FULL_BRANCH}\"\n\n"
		"// If you don't want this file to update/recompile, change to 1.\n"
		"#define RPCS3_GIT_VERSION_NO_UPDATE 0\n")

	if ("${GIT_VERSION_UPDATE}" EQUAL "1")
		file(WRITE ${GIT_VERSION_FILE} ${code_string})
	endif()
endfunction()
