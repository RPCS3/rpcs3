# x86emitter library

# library name
set(x86emitterName x86emitter)

# Clear default flags
set(CMAKE_C_FLAGS "")
set(CMAKE_CXX_FLAGS "")
set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS_DEBUG "")
set(CMAKE_C_FLAGS_DEVEL "")
set(CMAKE_CXX_FLAGS_DEVEL "")
set(CMAKE_C_FLAGS_RELEASE "")
set(CMAKE_CXX_FLAGS_RELEASE "")

# set common flags
set(CommonFlags 
	-pthread
	-fno-guess-branch-probability
	-fno-dse
	-fno-tree-dse
	-fno-strict-aliasing
	-m32
	-march=i486
	-msse
	-msse2
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
	add_definitions(${CommonFlags} ${OptimizationFlags} -DPCSX2_DEVBUILD -DPCSX2_DEVEL)
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
	tools.cpp
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
add_library(${x86emitterName} STATIC ${x86emitterSources} ${x86emitterHeaders})

# link target with wx
target_link_libraries(${x86emitterName} ${wxWidgets_LIBRARIES})

# Linker strip option
if (CMAKE_BUILD_STRIP)
    target_link_libraries(${x86emitterName} -s)
endif (CMAKE_BUILD_STRIP)
