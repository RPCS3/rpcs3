# Check that people use the good file
if(NOT TOP_CMAKE_WAS_SOURCED)
    message(FATAL_ERROR "
    You did not 'cmake' the good CMakeLists.txt file. Use the one in the top dir.
    It is advice to delete all wrongly generated cmake stuff => CMakeFiles & CMakeCache.txt")
endif(NOT TOP_CMAKE_WAS_SOURCED)

# Allow to build the shader files
if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/zzogl-pg/opengl/ZeroGSShaders")
    add_subdirectory(ZeroGSShaders)
endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/zzogl-pg/opengl/ZeroGSShaders")

# plugin name
set(Output zzogl-cg-0.3.0)

set(CommonFlags
    -pthread
    -DZEROGS_SSE2
    -fno-regmove
    -fno-strict-aliasing
    -Wstrict-aliasing # Allow to track strict aliasing issue.
    -Wunused-variable
    )

set(OptimizationFlags
    -O2
    -DNDEBUG
    )

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
    # add defines
    add_definitions(${CommonFlags} -g -Wall -D_DEBUG)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
    # add defines
    add_definitions(${CommonFlags} ${OptimizationFlags} -g -W -DZEROGS_DEVBUILD)
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
    # add defines
    add_definitions(${CommonFlags} ${OptimizationFlags} -W)
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# zzogl sources
set(zzoglSources
    GifTransfer.cpp
    GLWin32.cpp
    GLWinX11.cpp
    GSDump.cpp
    GSmain.cpp
    HostMemory.cpp
    Mem.cpp
    # memcpy_amd.cpp
    Mem_Swizzle.cpp
    Mem_Tables.cpp
    Profile.cpp
    rasterfont.cpp
    NewRegs.cpp
    Regs.cpp
    targets.cpp
    x86.cpp
    zerogs.cpp
    zpipe.cpp
    ZZClut.cpp
    ZZHacks.cpp
    ZZKeyboard.cpp
    ZZoglDrawing.cpp
    ZZLog.cpp
    ZZoglCreate.cpp
    ZZoglCRTC.cpp
    ZZoglFlush.cpp
    ZZoglFlushHack.cpp
    ZZoglSave.cpp
    ZZoglShaders.cpp
    ZZoglShoots.cpp
    ZZoglVB.cpp
    )

# zzogl headers
set(zzoglHeaders
    common.h
    CRC.h
    GifTransfer.h
    # glprocs.h
    GS.h
    GSDump.h
    HostMemory.h
    Mem.h
    Mem_Swizzle.h
    Mem_Transmit.h
    Profile.h
    rasterfont.h
    NewRegs.h
    Regs.h
    targets.h
    Util.h
    x86.h
    zerogs.h
    zpipe.h
    ZZClut.h
    ZZGl.h
    ZZHacks.h
    ZZoglDrawing.h
    ZZLog.h
    ZZoglCRTC.h
    ZZoglMath.h
    ZZoglShaders.h
    ZZoglShoots.h
    ZZoglVB.h
    )

# zzogl S sources
set(zzoglSSources
    x86-32.S)

# zzogl shader sources
set(zzoglShaderSources
    ctx0/ps2hw_ctx.fx
    ctx1/ps2hw_ctx.fx)

# zzogl Linux sources
set(zzoglLinuxSources
    Linux/Conf.cpp
    Linux/Linux.cpp)

# zzogl Linux headers
set(zzoglLinuxHeaders
    Linux/Linux.h)

# change language of .S-files to c++
set_source_files_properties(${zzoglSSources} PROPERTIES LANGUAGE CXX)

# add additional include directories
include_directories(.
    Linux)

# add library
add_library(${Output} SHARED
    ${zzoglSources}
    ${zzoglHeaders}
    ${zzoglSSources}
    ${zzoglShaderSources}
    ${zzoglLinuxSources}
    ${zzoglLinuxHeaders})

# Trick that allow to compile zzogl with GSOPEN2 and the replayer with GSOPEN
set_target_properties(${Output} PROPERTIES COMPILE_DEFINITIONS USE_GSOPEN2)

# link target with project internal libraries
target_link_libraries(${Output} Utilities)

# link target with Cg
target_link_libraries(${Output} ${CG_LIBRARIES})

# link target with glew
target_link_libraries(${Output} ${GLEW_LIBRARY})

# link target with opengl
target_link_libraries(${Output} ${OPENGL_LIBRARIES})

# link target with X11
target_link_libraries(${Output} ${X11_LIBRARIES})

# link target with jpeg
target_link_libraries(${Output} ${JPEG_LIBRARIES})

if(Linux)
	# link target with gtk2
	target_link_libraries(${Output} ${GTK2_LIBRARIES})
endif(Linux)

# link target with zlib
target_link_libraries(${Output} ${ZLIB_LIBRARIES})

# User flags options
if(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
    target_link_libraries(${Output} "${USER_CMAKE_LD_FLAGS}")
endif(NOT USER_CMAKE_LD_FLAGS STREQUAL "")

if(PACKAGE_MODE)
    install(TARGETS ${Output} DESTINATION ${PLUGIN_DIR})
    install(FILES ${PROJECT_SOURCE_DIR}/plugins/zzogl-pg/opengl/ps2hw.dat DESTINATION ${PLUGIN_DIR})
else(PACKAGE_MODE)
    install(TARGETS ${Output} DESTINATION ${CMAKE_SOURCE_DIR}/bin/plugins)
    install(FILES ${PROJECT_SOURCE_DIR}/plugins/zzogl-pg/opengl/ps2hw.dat DESTINATION ${CMAKE_SOURCE_DIR}/bin/plugins)
endif(PACKAGE_MODE)

################################### Replay Loader
set(Replay pcsx2_ZZCGReplayLoader)
set(Static zzogl-cg-static)

add_library(${Static} STATIC
    ${zzoglSources}
    ${zzoglHeaders}
    ${zzoglSSources}
    ${zzoglShaderSources}
    ${zzoglLinuxSources}
    ${zzoglLinuxHeaders})

target_link_libraries(${Static} Utilities)
target_link_libraries(${Static} ${CG_LIBRARIES})
target_link_libraries(${Static} ${GLEW_LIBRARY})
target_link_libraries(${Static} ${OPENGL_LIBRARIES})
target_link_libraries(${Static} ${X11_LIBRARIES})
target_link_libraries(${Static} ${JPEG_LIBRARIES})

add_executable(${Replay} linux_replay.cpp)

target_link_libraries(${Replay} ${Static})
target_link_libraries(${Replay} Utilities)
target_link_libraries(${Replay} ${CG_LIBRARIES})
target_link_libraries(${Replay} ${GLEW_LIBRARY})
target_link_libraries(${Replay} ${OPENGL_LIBRARIES})
target_link_libraries(${Replay} ${X11_LIBRARIES})
target_link_libraries(${Replay} ${JPEG_LIBRARIES})

target_link_libraries(${Replay} ${GTK2_LIBRARIES})
target_link_libraries(${Replay} ${ZLIB_LIBRARIES})

if(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
    target_link_libraries(${Replay} "${USER_CMAKE_LD_FLAGS}")
endif(NOT USER_CMAKE_LD_FLAGS STREQUAL "")

if(PACKAGE_MODE)
    install(TARGETS ${Replay} DESTINATION bin)
else(PACKAGE_MODE)
    install(TARGETS ${Replay} DESTINATION ${CMAKE_SOURCE_DIR}/bin)
endif(PACKAGE_MODE)
