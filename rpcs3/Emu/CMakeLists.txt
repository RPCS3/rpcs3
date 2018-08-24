add_subdirectory(Audio)
add_subdirectory(Cell)
add_subdirectory(CPU)
add_subdirectory(Io)
add_subdirectory(Memory)
add_subdirectory(RSX)

file(GLOB SRC_FILES "*.cpp")

add_library(rpcs3_emu ${SRC_FILES})

target_link_libraries(rpcs3_emu
	PUBLIC
		rpcs3_audio rpcs3_cell rpcs3_cpu rpcs3_io rpcs3_memory rpcs3_rsx
		rpcs3_utilities_jit
	PRIVATE
		rpcs3_stdafx
		3rdparty::zlib 3rdparty::yaml-cpp
)
