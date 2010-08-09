# Check that people use the good file
if(NOT TOP_CMAKE_WAS_SOURCED)
    message(FATAL_ERROR "
    You did not 'cmake' the good CMakeLists.txt file. Use the one in the top dir.
    It is advice to delete all wrongly generated cmake stuff => CMakeFiles & CMakeCache.txt")
endif(NOT TOP_CMAKE_WAS_SOURCED)


# library name
set(Output x86emitter)

# set common flags
set(CommonFlags 
	-pthread
    -fvisibility=hidden
	-fno-guess-branch-probability
	-fno-dse
	-fno-tree-dse
	-fno-strict-aliasing
	-pipe
	-Wno-format
	-Wno-unused-parameter
	-Wno-unused-value
	-Wunused-variable)

# set warning flags
set(DebugFlags
	-W
	-g)

# set optimization flags
set(OptimizationFlags
	-falign-functions
	-falign-jumps
	-falign-labels
	-falign-loops
	-fcaller-saves
	-fcprop-registers
	-fcrossjumping
	-fcse-follow-jumps
	-fcse-skip-blocks
	-fdefer-pop
	-fdelete-null-pointer-checks
	-fgcse
	-fgcse-lm
	-fif-conversion
	-fif-conversion2
	-fmerge-constants
	-foptimize-sibling-calls
	-fpeephole2
	-fregmove
	-freorder-blocks
	-freorder-functions 
	-frerun-cse-after-loop
	-fsched-interblock
	-fsched-spec
	-fstrict-overflow
	-fthread-jumps
	-ftree-ccp
	-ftree-ch
	-ftree-copyrename
	-ftree-dce
	-ftree-dominator-opts
	-ftree-fre
	-ftree-lrs
	-ftree-pre
	-ftree-sra
	-ftree-ter
	-ftree-vrp
	-funit-at-a-time)

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)

	# add defines
	add_definitions(${CommonFlags} ${DebugFlags} -DPCSX2_DEVBUILD -DPCSX2_DEBUG)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)

	# add defines
	add_definitions(${CommonFlags} ${OptimizationFlags} -DPCSX2_DEVBUILD)
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	
	# add defines
	add_definitions(${CommonFlags} ${OptimizationFlags})
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# variable with all sources of this library
set(x86emitterSources
	../../include/x86emitter/inlines.inl
	3dnow.cpp
	cpudetect.cpp
	fpu.cpp
	groups.cpp
	jmp.cpp
	legacy.cpp
	legacy_sse.cpp
	LnxCpuDetect.cpp
	movs.cpp
	PrecompiledHeader.cpp
	simd.cpp
	x86emitter.cpp)

# variable with all headers of this library
set(x86emitterHeaders
	../../include/x86emitter/implement/dwshift.h
	../../include/x86emitter/implement/group1.h
	../../include/x86emitter/implement/group2.h
	../../include/x86emitter/implement/group3.h
	../../include/x86emitter/implement/helpers.h
	../../include/x86emitter/implement/incdec.h
	../../include/x86emitter/implement/jmpcall.h
	../../include/x86emitter/implement/movs.h
	../../include/x86emitter/implement/simd_arithmetic.h
	../../include/x86emitter/implement/simd_comparisons.h
	../../include/x86emitter/implement/simd_helpers.h
	../../include/x86emitter/implement/simd_moremovs.h
	../../include/x86emitter/implement/simd_shufflepack.h
	../../include/x86emitter/implement/simd_templated_helpers
	../../include/x86emitter/implement/test.h
	../../include/x86emitter/implement/xchg.h
	../../include/x86emitter/instructions.h
	../../include/x86emitter/internal.h
	../../include/x86emitter/legacy_instructions.h
	../../include/x86emitter/legacy_internal.h
	../../include/x86emitter/legacy_types.h
	../../include/x86emitter/macros.h
	../../include/x86emitter/sse_helpers.h
	../../include/x86emitter/tools.h
	../../include/x86emitter/x86emitter.h
	../../include/x86emitter/x86types.h
	PrecompiledHeader.h)

# add library
add_library(${Output} STATIC ${x86emitterSources} ${x86emitterHeaders})

# link target with wx
target_link_libraries(${Output} ${wxWidgets_LIBRARIES})

# User flags options
if(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
    target_link_libraries(${Output} "${USER_CMAKE_LD_FLAGS}")
endif(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
