if(WITH_LLVM)
	if(BUILD_LLVM_SUBMODULE)
		message(STATUS "LLVM will be built from the submodule.")

		set(LLVM_TARGETS_TO_BUILD "X86" CACHE INTERNAL "")
		option(LLVM_BUILD_RUNTIME OFF)
		option(LLVM_BUILD_TOOLS OFF)
		option(LLVM_INCLUDE_BENCHMARKS OFF)
		option(LLVM_INCLUDE_DOCS OFF)
		option(LLVM_INCLUDE_EXAMPLES OFF)
		option(LLVM_INCLUDE_TESTS OFF)
		option(LLVM_INCLUDE_TOOLS OFF)
		option(LLVM_INCLUDE_UTILS OFF)
		option(WITH_POLLY OFF)
		option(LLVM_CCACHE_BUILD ON)

		set(CXX_FLAGS_OLD ${CMAKE_CXX_FLAGS})

		if (MSVC)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")
		endif()

		# LLVM needs to be built out-of-tree
		add_subdirectory(${CMAKE_SOURCE_DIR}/llvm ${CMAKE_CURRENT_BINARY_DIR}/llvm_build EXCLUDE_FROM_ALL)
		set(LLVM_DIR "${CMAKE_CURRENT_BINARY_DIR}/llvm_build/lib/cmake/llvm/")

		set(CMAKE_CXX_FLAGS ${CXX_FLAGS_OLD})

		# now tries to find LLVM again
		find_package(LLVM 11.0 CONFIG)
		if(NOT LLVM_FOUND)
			message(FATAL_ERROR "Couldn't build LLVM from the submodule. You might need to run `git submodule update --init`")
		endif()

	else()
		message(STATUS "Using prebuilt LLVM")

		if (LLVM_DIR AND NOT IS_ABSOLUTE "${LLVM_DIR}")
			# change relative LLVM_DIR to be relative to the source dir
			set(LLVM_DIR ${CMAKE_SOURCE_DIR}/${LLVM_DIR})
		endif()

		find_package(LLVM 11.0 CONFIG)

		if (NOT LLVM_FOUND)
			if (LLVM_VERSION AND LLVM_VERSION_MAJOR LESS 11)
				message(FATAL_ERROR "Found LLVM version ${LLVM_VERSION}. Required version 11.0. \
														 Enable BUILD_LLVM_SUBMODULE option to build LLVM from included as a git submodule.")
			endif()

			message(FATAL_ERROR "Can't find LLVM libraries from the CMAKE_PREFIX_PATH path or LLVM_DIR. \
													 Enable BUILD_LLVM_SUBMODULE option to build LLVM from included as a git submodule.")
		endif()
	endif()

	set(LLVM_LIBS LLVMMCJIT LLVMX86CodeGen LLVMX86AsmParser)

	add_library(3rdparty_llvm INTERFACE)
	target_link_libraries(3rdparty_llvm INTERFACE ${LLVM_LIBS})
	target_include_directories(3rdparty_llvm INTERFACE ${LLVM_INCLUDE_DIRS})
	target_compile_definitions(3rdparty_llvm INTERFACE ${LLVM_DEFINITIONS} -DLLVM_AVAILABLE)

	add_library(3rdparty::llvm ALIAS 3rdparty_llvm)
else()
	add_library(3rdparty::llvm ALIAS 3rdparty_dummy_lib)
endif()
