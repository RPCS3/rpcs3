if(WITH_LLVM)
	check_cxx_compiler_flag("-msse -msse2 -mcx16" COMPILER_X86)
	check_cxx_compiler_flag("-march=armv8-a+lse" COMPILER_ARM)

	if(BUILD_LLVM)
		message(STATUS "LLVM will be built from the submodule.")

		if (ANDROID)
			if (COMPILER_ARM)
				set(LLVM_TARGETS_TO_BUILD "AArch64" CACHE STRING "Semicolon-separated list of targets to build, or \"all\".")
				set(LLVM_TARGET_ARCH "${CMAKE_SYSTEM_PROCESSOR}-none-linux-android${ANDROID_NATIVE_API_LEVEL}" CACHE STRING "")
			else()
				set(LLVM_TARGETS_TO_BUILD "X86" CACHE STRING "Semicolon-separated list of targets to build, or \"all\".")
			endif()
		else()
			set(LLVM_TARGETS_TO_BUILD "AArch64;X86" CACHE STRING "Semicolon-separated list of targets to build, or \"all\".")
		endif()

		option(LLVM_BUILD_RUNTIME "Build the LLVM runtime libraries." OFF)
		option(LLVM_BUILD_TOOLS "Build the LLVM tools. If OFF, just generate build targets." OFF)
		option(LLVM_INCLUDE_BENCHMARKS "Generate benchmark targets. If OFF, benchmarks can't be built." OFF)
		option(LLVM_INCLUDE_DOCS "Generate build targets for llvm documentation." OFF)
		option(LLVM_INCLUDE_EXAMPLES "Generate build targets for the LLVM examples" OFF)
		option(LLVM_INCLUDE_TESTS "Generate build targets for the LLVM unit tests." OFF)
		option(LLVM_INCLUDE_TOOLS "Generate build targets for the LLVM tools." OFF)
		option(LLVM_INCLUDE_UTILS "Generate build targets for the LLVM utils." OFF)
		option(LLVM_CCACHE_BUILD "Set to ON for a ccache enabled build" OFF)
		set(LLVM_ENABLE_WARNINGS OFF CACHE BOOL "Enable compiler warnings.")

		# For Windows x86 (not Windows AArch64).
		# Check on MSVC is needed due to COMPILER_X86, COMPILER_ARM etc. are not set/supported by the MSVC compiler, if used.
		# Furthermore, the MSVC compiler is not available/supported on Windows AArch64
		if(WIN32 AND (COMPILER_X86 OR MSVC))
			set(LLVM_USE_INTEL_JITEVENTS ON)
		endif()

		if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
			if(COMPILER_X86)
				set(LLVM_USE_INTEL_JITEVENTS ON)
			endif()
			set(LLVM_USE_PERF ON)
		endif()

		if (MSVC)
			add_compile_definitions("$<$<COMPILE_LANGUAGE:CXX>:_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS>")
		endif()

		# LLVM needs to be built out-of-tree
		add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/llvm/llvm ${CMAKE_CURRENT_BINARY_DIR}/llvm_build EXCLUDE_FROM_ALL)
		set(LLVM_DIR "${CMAKE_CURRENT_BINARY_DIR}/llvm_build/lib/cmake/llvm/")

		set(STATIC_LINK_LLVM ON CACHE BOOL "Link against LLVM statically. This will get set to ON if you build LLVM from the submodule." FORCE)

		find_package(LLVM 19.1 CONFIG)
		if(NOT LLVM_FOUND)
			message(FATAL_ERROR "Couldn't build LLVM from the submodule. You might need to run `git submodule update --init`")
		endif()
	else()
		message(STATUS "Using prebuilt or system LLVM")

		if (LLVM_DIR AND NOT IS_ABSOLUTE "${LLVM_DIR}")
			# change relative LLVM_DIR to be relative to the source dir
			set(LLVM_DIR ${CMAKE_SOURCE_DIR}/${LLVM_DIR})
		endif()

		find_package(LLVM CONFIG)

		if (NOT LLVM_FOUND)
			message(FATAL_ERROR "Can't find LLVM libraries from the CMAKE_PREFIX_PATH path or LLVM_DIR. Enable BUILD_LLVM option to build LLVM from included as a git submodule.")
		endif()
		if (LLVM_VERSION VERSION_LESS 18)
			message(FATAL_ERROR "Found LLVM version ${LLVM_VERSION}. Required version 18 or above. Enable BUILD_LLVM option to build LLVM from included as a git submodule.")
		endif()
	endif()

	if (STATIC_LINK_LLVM)
		if (NOT DEFINED LLVM_TARGETS_TO_BUILD)
			if(COMPILER_ARM)
				set(LLVM_TARGETS_TO_BUILD "AArch64" CACHE STRING "Semicolon-separated list of targets to build, or \"all\".")
			else()
				set(LLVM_TARGETS_TO_BUILD "X86" CACHE STRING "Semicolon-separated list of targets to build, or \"all\".")
			endif()
		endif()

		# For Windows x86 (not Windows AArch64) only when BUILD_LLVM is enabled and
		# for Linux x86 (not Linux AArch64) even if BUILD_LLVM is disabled (precompiled llvm used)
		if(LLVM_USE_INTEL_JITEVENTS OR (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND COMPILER_X86))
			list (APPEND LLVM_ADDITIONAL_LIBS IntelJITEvents)
		endif()

		# For Linux even if BUILD_LLVM is disabled (precompiled llvm used)
		if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
			list (APPEND LLVM_ADDITIONAL_LIBS PerfJITEvents)
		endif()

		llvm_map_components_to_libnames(LLVM_LIBS
			${LLVM_TARGETS_TO_BUILD}
			${LLVM_ADDITIONAL_LIBS}
			Core
			ExecutionEngine
			MCJIT
			Passes
		)
	else()
		set(LLVM_LIBS LLVM)
	endif()

	add_library(3rdparty_llvm INTERFACE)
	target_link_libraries(3rdparty_llvm INTERFACE ${LLVM_LIBS})
	target_include_directories(3rdparty_llvm INTERFACE ${LLVM_INCLUDE_DIRS})
	separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
	target_compile_definitions(3rdparty_llvm INTERFACE ${LLVM_DEFINITIONS_LIST} LLVM_AVAILABLE)

	add_library(3rdparty::llvm ALIAS 3rdparty_llvm)
else()
	add_library(3rdparty::llvm ALIAS 3rdparty_dummy_lib)
endif()
