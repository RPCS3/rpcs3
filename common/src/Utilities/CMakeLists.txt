# Utilities library

# library name
set(UtilitiesName Utilities)

# variable with all sources of this library
set(UtilitiesSources
	../../include/Utilities/EventSource.inl
	../../include/Utilities/TlsVariable.inl
	AlignedMalloc.cpp
	CheckedStaticBox.cpp
	Console.cpp
	EventSource.cpp
	Exceptions.cpp
	HashTools.cpp
	Linux/LnxHostSys.cpp
	Linux/LnxMisc.cpp
	Linux/LnxThreads.cpp
	Mutex.cpp
	PathUtils.cpp
	PrecompiledHeader.cpp
	pxCheckBox.cpp
	pxRadioPanel.cpp
	pxStaticText.cpp
	Semaphore.cpp
	StringHelpers.cpp
	ThreadTools.cpp
	vssprintf.cpp
	wxGuiTools.cpp
	wxHelpers.cpp
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
	../../include/Utilities/StringHelpers.h
	../../include/Utilities/Threading.h
	../../include/Utilities/wxBaseTools.h
	../../include/Utilities/wxGuiTools.h
	PrecompiledHeader.h)

# change language of .S-files to c++
set_source_files_properties(${UtilitiesSSources} PROPERTIES LANGUAGE CXX)

# add library
add_library(${UtilitiesName} STATIC ${UtilitiesSources} ${UtilitiesHeaders} ${UtilitiesSSources})

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	set(CMAKE_C_FLAGS_DEBUG "-W -g -march=i486 -Wno-format -Wno-unused-parameter -Wno-unused-value -Wunused-variable -fno-guess-branch-probability -fno-dse -fno-tree-dse -fno-strict-aliasing -pipe -msse -msse2 -m32")

	set(CMAKE_CXX_FLAGS_DEBUG "-W -g -march=i486 -Wno-format -Wno-unused-parameter -Wno-unused-value -Wunused-variable -fno-guess-branch-probability -fno-dse -fno-tree-dse -fno-strict-aliasing -pipe -msse -msse2 -m32")
	
	# add defines
	add_definitions(-DPCSX2_DEVBUILD -DPCSX2_DEBUG)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
	set(CMAKE_C_FLAGS_DEVEL "-fdefer-pop -fcprop-registers -fif-conversion -fif-conversion2 -ftree-ccp -ftree-dce -ftree-dominator-opts -ftree-ter -ftree-lrs -ftree-sra -ftree-copyrename -ftree-fre -ftree-ch -funit-at-a-time -fmerge-constants -fthread-jumps -fcrossjumping -foptimize-sibling-calls -fcse-follow-jumps -fcse-skip-blocks -fgcse -fgcse-lm -frerun-cse-after-loop -fcaller-saves -fpeephole2 -fsched-interblock -fsched-spec -fregmove -fstrict-overflow -fdelete-null-pointer-checks -freorder-blocks -freorder-functions -falign-functions -falign-jumps -falign-loops -falign-labels -ftree-vrp -ftree-pre -march=i486 -Wno-format -Wno-unused-parameter -Wno-unused-value -Wunused-variable -fno-guess-branch-probability -fno-dse -fno-tree-dse -fno-strict-aliasing -pipe -msse -msse2 -m32")
	
	set(CMAKE_CXX_FLAGS_DEVEL "-fdefer-pop -fcprop-registers -fif-conversion -fif-conversion2 -ftree-ccp -ftree-dce -ftree-dominator-opts -ftree-ter -ftree-lrs -ftree-sra -ftree-copyrename -ftree-fre -ftree-ch -funit-at-a-time -fmerge-constants -fthread-jumps -fcrossjumping -foptimize-sibling-calls -fcse-follow-jumps -fcse-skip-blocks -fgcse -fgcse-lm -frerun-cse-after-loop -fcaller-saves -fpeephole2 -fsched-interblock -fsched-spec -fregmove -fstrict-overflow -fdelete-null-pointer-checks -freorder-blocks -freorder-functions -falign-functions -falign-jumps -falign-loops -falign-labels -ftree-vrp -ftree-pre -march=i486 -Wno-format -Wno-unused-parameter -Wno-unused-value -Wunused-variable -fno-guess-branch-probability -fno-dse -fno-tree-dse -fno-strict-aliasing -pipe -msse -msse2 -m32")
	
	# add defines
	add_definitions(-DPCSX2_DEVBUILD -DPCSX2_DEBUG)
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
	set(CMAKE_C_FLAGS_RELEASE "-fdefer-pop -fcprop-registers -fif-conversion -fif-conversion2 -ftree-ccp -ftree-dce -ftree-dominator-opts -ftree-ter -ftree-lrs -ftree-sra -ftree-copyrename -ftree-fre -ftree-ch -funit-at-a-time -fmerge-constants -fthread-jumps -fcrossjumping -foptimize-sibling-calls -fcse-follow-jumps -fcse-skip-blocks -fgcse -fgcse-lm -frerun-cse-after-loop -fcaller-saves -fpeephole2 -fsched-interblock -fsched-spec -fregmove -fstrict-overflow -fdelete-null-pointer-checks -freorder-blocks -freorder-functions -falign-functions -falign-jumps -falign-loops -falign-labels -ftree-vrp -ftree-pre -march=i486 -Wno-format -Wno-unused-parameter -Wno-unused-value -Wunused-variable -fno-guess-branch-probability -fno-dse -fno-tree-dse -fno-strict-aliasing -pipe -msse -msse2 -m32 -s")

	set(CMAKE_CXX_FLAGS_RELEASE "-fdefer-pop -fcprop-registers -fif-conversion -fif-conversion2 -ftree-ccp -ftree-dce -ftree-dominator-opts -ftree-ter -ftree-lrs -ftree-sra -ftree-copyrename -ftree-fre -ftree-ch -funit-at-a-time -fmerge-constants -fthread-jumps -fcrossjumping -foptimize-sibling-calls -fcse-follow-jumps -fcse-skip-blocks -fgcse -fgcse-lm -frerun-cse-after-loop -fcaller-saves -fpeephole2 -fsched-interblock -fsched-spec -fregmove -fstrict-overflow -fdelete-null-pointer-checks -freorder-blocks -freorder-functions -falign-functions -falign-jumps -falign-loops -falign-labels -ftree-vrp -ftree-pre -march=i486 -Wno-format -Wno-unused-parameter -Wno-unused-value -Wunused-variable -fno-guess-branch-probability -fno-dse -fno-tree-dse -fno-strict-aliasing -pipe -msse -msse2 -m32 -s")
	
	# add defines
	add_definitions()
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# link target with wx
target_link_libraries(${UtilitiesName} ${wxWidgets_LIBRARIES})

# add additional include directories
include_directories(.)

