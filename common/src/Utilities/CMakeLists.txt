# Check that people use the good file
if(NOT TOP_CMAKE_WAS_SOURCED)
    message(FATAL_ERROR "
    You did not 'cmake' the good CMakeLists.txt file. Use the one in the top dir.
    It is advice to delete all wrongly generated cmake stuff => CMakeFiles & CMakeCache.txt")
endif(NOT TOP_CMAKE_WAS_SOURCED)


# library name
set(Output Utilities)

# set common flags
set(CommonFlags 
	-pthread
    -fvisibility=hidden
	-fno-dse
	-fno-guess-branch-probability
	-fno-strict-aliasing
	-fno-tree-dse
	-pipe
	-Wno-format
	-Wno-unused-parameter
	-Wno-unused-value
	-Wunused-variable)

# set warning flags
set(DebugFlags
	-g
	-W)

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
	add_definitions(${CommonFlags} ${DebugFlags} -DPCSX2_DEBUG -DPCSX2_DEVBUILD)
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
set(UtilitiesSources
	../../include/Utilities/FixedPointTypes.inl
	../../include/Utilities/EventSource.inl
    ../../include/Utilities/SafeArray.inl
    ../../include/Utilities/TlsVariable.inl
	AlignedMalloc.cpp
	CheckedStaticBox.cpp
	Console.cpp
	EventSource.cpp
	Exceptions.cpp
	FastFormatString.cpp
	HashTools.cpp
	IniInterface.cpp
	Linux/LnxHostSys.cpp
	Linux/LnxMisc.cpp
	Linux/LnxThreads.cpp
	Mutex.cpp
	PathUtils.cpp
	PrecompiledHeader.cpp
	pxCheckBox.cpp
	pxRadioPanel.cpp
	pxStaticText.cpp
	pxTextStream.cpp
	pxTranslate.cpp
	pxWindowTextWriter.cpp
	Semaphore.cpp
	StringHelpers.cpp
	ThreadingDialogs.cpp
	ThreadTools.cpp
	vssprintf.cpp
	wxAppWithHelpers.cpp
	wxGuiTools.cpp
	wxHelpers.cpp
	x86/MemcpyVibes.cpp
#	x86/MemcpyFast.cpp
	)

# collect .S files
set(UtilitiesSSources
	x86/MemcpyFast.S)

# variable with all headers of this library
set(UtilitiesHeaders
	../../include/Utilities/Assertions.h
	../../include/Utilities/CheckedStaticBox.h
	../../include/Utilities/Console.h
	../../include/Utilities/Dependencies.h
	../../include/Utilities/EventSource.h
	../../include/Utilities/Exceptions.h
	../../include/Utilities/FixedPointTypes.h
	../../include/Utilities/General.h
	../../include/Utilities/HashMap.h
	../../include/Utilities/lnx_memzero.h
	../../include/Utilities/MemcpyFast.h
	../../include/Utilities/Path.h
	../../include/Utilities/pxCheckBox.h
	../../include/Utilities/pxRadioPanel.h
	../../include/Utilities/pxStaticText.h
	../../include/Utilities/RedtapeWindows.h
	../../include/Utilities/SafeArray.h
	../../include/Utilities/ScopedPtr.h
	../../include/Utilities/ScopedPtrMT.h
	../../include/Utilities/StringHelpers.h
	../../include/Utilities/Threading.h
	../../include/Utilities/ThreadingDialogs.h
	../../include/Utilities/TraceLog.h
	../../include/Utilities/wxAppWithHelpers.h
	../../include/Utilities/wxBaseTools.h
	../../include/Utilities/wxGuiTools.h
	PrecompiledHeader.h)

include_directories(.)

# change language of .S-files to c++
set_source_files_properties(${UtilitiesSSources} PROPERTIES LANGUAGE CXX)

# add library
add_library(${Output} STATIC ${UtilitiesSources} ${UtilitiesHeaders} ${UtilitiesSSources})

# link target with wx
target_link_libraries(${Output} ${wxWidgets_LIBRARIES})

# User flags options
if(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
    target_link_libraries(${Output} "${USER_CMAKE_LD_FLAGS}")
endif(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
