if(NOT WITHOUT_LLVM)
	if (EXISTS "${CMAKE_SOURCE_DIR}/llvmlibs")
		find_package(LLVM 7.0 CONFIG)
	endif()
	if(NOT LLVM_FOUND)
		message("LLVM will be built from the submodule.")

		set(LLVM_TARGETS_TO_BUILD "X86" CACHE INTERNAL "")
		option(LLVM_BUILD_RUNTIME OFF)
		option(LLVM_BUILD_TOOLS OFF)
		option(LLVM_INCLUDE_DOCS OFF)
		option(LLVM_INCLUDE_EXAMPLES OFF)
		option(LLVM_INCLUDE_TESTS OFF)
		option(LLVM_INCLUDE_TOOLS OFF)
		option(LLVM_INCLUDE_UTILS OFF)
		option(WITH_POLLY OFF)
		option(LLVM_ENABLE_CXX1Z TRUE)

		set(CXX_FLAGS_OLD ${CMAKE_CXX_FLAGS})

		if (MSVC)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")
		endif()

		# LLVM needs to be built out-of-tree
		add_subdirectory(${CMAKE_SOURCE_DIR}/llvm ${CMAKE_CURRENT_BINARY_DIR}/llvm_build EXCLUDE_FROM_ALL)
		set(LLVM_DIR "${CMAKE_CURRENT_BINARY_DIR}/llvm_build/lib/cmake/llvm/")

		set(CMAKE_CXX_FLAGS ${CXX_FLAGS_OLD})

		# now tries to find LLVM again
		find_package(LLVM 7.0 CONFIG)
		if(NOT LLVM_FOUND)
			message(FATAL_ERROR "Couldn't build LLVM from the submodule. You might need to run `git submodule update --init`")
		endif()
	endif()

	set(LLVM_LIBS LLVMMCJIT LLVMX86CodeGen)

	add_library(3rdparty_llvm INTERFACE)
	target_link_libraries(3rdparty_llvm INTERFACE ${LLVM_LIBS})
	target_include_directories(3rdparty_llvm INTERFACE ${LLVM_INCLUDE_DIRS})
	target_compile_definitions(3rdparty_llvm INTERFACE ${LLVM_DEFINITIONS} -DLLVM_AVAILABLE)

	add_library(3rdparty::llvm ALIAS 3rdparty_llvm)
else()
	add_library(3rdparty::llvm ALIAS 3rdparty_dummy_lib)
endif()