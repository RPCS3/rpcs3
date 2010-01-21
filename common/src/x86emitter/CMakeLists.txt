# x86emitter library

# library name
set(x86emitterName x86emitter)

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
#	../../include/x86emitter/implement/xmm/
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

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	set(CMAKE_C_FLAGS_DEBUG "-march=i486 -Wno-format -Wno-unused-parameter -Wno-unused-value -Wunused-variable -fno-guess-branch-probability -fno-dse -fno-tree-dse -fno-strict-aliasing -pipe -msse -msse2 -m32 -W -g")

	set(CMAKE_CXX_FLAGS_DEBUG "-march=i486 -Wno-format -Wno-unused-parameter -Wno-unused-value -Wunused-variable -fno-guess-branch-probability -fno-dse -fno-tree-dse -fno-strict-aliasing -pipe -msse -msse2 -m32 -W -g")

	# add defines
	add_definitions(-DPCSX2_DEVBUILD -DPCSX2_DEBUG)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	set(CMAKE_C_FLAGS_DEVEL "-march=i486 -Wno-format -Wno-unused-parameter -Wno-unused-value -Wunused-variable -fno-guess-branch-probability -fno-dse -fno-tree-dse -fno-strict-aliasing -pipe -msse -msse2 -m32 -fdefer-pop -fcprop-registers -fif-conversion -fif-conversion2 -ftree-ccp -ftree-dce -ftree-dominator-opts -ftree-ter -ftree-lrs -ftree-sra -ftree-copyrename -ftree-fre -ftree-ch -funit-at-a-time -fmerge-constants -fthread-jumps -fcrossjumping -foptimize-sibling-calls -fcse-follow-jumps -fcse-skip-blocks -fgcse  -fgcse-lm -frerun-cse-after-loop -fcaller-saves -fpeephole2 -fsched-interblock  -fsched-spec -fregmove -fstrict-overflow -fdelete-null-pointer-checks -freorder-blocks  -freorder-functions -falign-functions  -falign-jumps -falign-loops  -falign-labels -ftree-vrp -ftree-pre")

	set(CMAKE_CXX_FLAGS_DEVEL "-march=i486 -Wno-format -Wno-unused-parameter -Wno-unused-value -Wunused-variable -fno-guess-branch-probability -fno-dse -fno-tree-dse -fno-strict-aliasing -pipe -msse -msse2 -m32 -fdefer-pop -fcprop-registers -fif-conversion -fif-conversion2 -ftree-ccp -ftree-dce -ftree-dominator-opts -ftree-ter -ftree-lrs -ftree-sra -ftree-copyrename -ftree-fre -ftree-ch -funit-at-a-time -fmerge-constants -fthread-jumps -fcrossjumping -foptimize-sibling-calls -fcse-follow-jumps -fcse-skip-blocks -fgcse  -fgcse-lm -frerun-cse-after-loop -fcaller-saves -fpeephole2 -fsched-interblock  -fsched-spec -fregmove -fstrict-overflow -fdelete-null-pointer-checks -freorder-blocks  -freorder-functions -falign-functions  -falign-jumps -falign-loops  -falign-labels -ftree-vrp -ftree-pre")

	# add defines
	add_definitions(-DPCSX2_DEVBUILD -DPCSX2_DEBUG)
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	set(CMAKE_C_FLAGS_RELEASE "-march=i486 -Wno-format -Wno-unused-parameter -Wno-unused-value -Wunused-variable -fno-guess-branch-probability -fno-dse -fno-tree-dse -fno-strict-aliasing -pipe -msse -msse2 -m32 -fdefer-pop -fcprop-registers -fif-conversion -fif-conversion2 -ftree-ccp -ftree-dce -ftree-dominator-opts -ftree-ter -ftree-lrs -ftree-sra -ftree-copyrename -ftree-fre -ftree-ch -funit-at-a-time -fmerge-constants -fthread-jumps -fcrossjumping -foptimize-sibling-calls -fcse-follow-jumps -fcse-skip-blocks -fgcse  -fgcse-lm -frerun-cse-after-loop -fcaller-saves -fpeephole2 -fsched-interblock  -fsched-spec -fregmove -fstrict-overflow -fdelete-null-pointer-checks -freorder-blocks  -freorder-functions -falign-functions  -falign-jumps -falign-loops  -falign-labels -ftree-vrp -ftree-pre -s")

	set(CMAKE_CXX_FLAGS_RELEASE "-march=i486 -Wno-format -Wno-unused-parameter -Wno-unused-value -Wunused-variable -fno-guess-branch-probability -fno-dse -fno-tree-dse -fno-strict-aliasing -pipe -msse -msse2 -m32 -fdefer-pop -fcprop-registers -fif-conversion -fif-conversion2 -ftree-ccp -ftree-dce -ftree-dominator-opts -ftree-ter -ftree-lrs -ftree-sra -ftree-copyrename -ftree-fre -ftree-ch -funit-at-a-time -fmerge-constants -fthread-jumps -fcrossjumping -foptimize-sibling-calls -fcse-follow-jumps -fcse-skip-blocks -fgcse  -fgcse-lm -frerun-cse-after-loop -fcaller-saves -fpeephole2 -fsched-interblock  -fsched-spec -fregmove -fstrict-overflow -fdelete-null-pointer-checks -freorder-blocks  -freorder-functions -falign-functions  -falign-jumps -falign-loops  -falign-labels -ftree-vrp -ftree-pre -s")
	
	# add defines
	add_definitions()
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# link target with wx
target_link_libraries(${x86emitterName} ${wxWidgets_LIBRARIES})

# add additional include directories
include_directories(../../include
					../../include/x86emitter)

