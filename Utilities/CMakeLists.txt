set(CORE_SRC_FILES
	File.cpp
	Log.cpp
	Thread.cpp
	StrFmt.cpp
	VirtualMemory.cpp
)

add_library(rpcs3_core_utilities ${CORE_SRC_FILES})

# this is kinda stupid but can't fix without changing directory structure
target_include_directories(rpcs3_core_utilities PUBLIC "..")

# required for stdafx.h
target_include_directories(rpcs3_core_utilities PUBLIC "../rpcs3")

target_link_libraries(rpcs3_core_utilities PRIVATE 3rdparty::zlib)


set(SRC_FILES
	bin_patch.cpp
	cond.cpp
	Config.cpp
	dynamic_library.cpp
	GDBDebugServer.cpp
	Log.cpp
	LUrlParser.cpp
	mutex.cpp
	rXml.cpp
	sema.cpp
	sysinfo.cpp
	version.cpp
)

add_library(rpcs3_utilities ${SRC_FILES})

target_link_libraries(rpcs3_utilities PUBLIC rpcs3_core_utilities pugixml 3rdparty::gsl)
target_link_libraries(rpcs3_utilities PRIVATE rpcs3_crypto yaml-cpp 3rdparty::zlib)


add_library(rpcs3_utilities_jit JIT.cpp)

target_link_libraries(rpcs3_utilities_jit
	PUBLIC
		3rdparty::llvm 3rdparty::asmjit
		3rdparty::zlib)

# required for "restore_new.h"
target_include_directories(rpcs3_utilities_jit
  PRIVATE
		"../rpcs3")

if(NOT MSVC)
  set_source_files_properties(JIT.cpp PROPERTIES COMPILE_FLAGS -fno-rtti)
endif()
