set(SRC_FILES
	CPUThread.cpp
	CPUTranslator.cpp
)

add_library(rpcs3_cpu ${SRC_FILES})
target_link_libraries(rpcs3_cpu
	PRIVATE
		rpcs3_stdafx
	PUBLIC
		3rdparty::llvm)
