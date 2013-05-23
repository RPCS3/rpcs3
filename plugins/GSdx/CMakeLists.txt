# Check that people use the good file
if(NOT TOP_CMAKE_WAS_SOURCED)
    message(FATAL_ERROR "
    You did not 'cmake' the good CMakeLists.txt file. Use the one in the top dir.
    It is advice to delete all wrongly generated cmake stuff => CMakeFiles & CMakeCache.txt")
endif(NOT TOP_CMAKE_WAS_SOURCED)


# plugin name
set(Output GSdx-0.1.16)

set(CommonFlags
    -D_LINUX
    -fno-operator-names
    -mpreferred-stack-boundary=2
    -mfpmath=sse
    #-Wstrict-aliasing # Allow to track strict aliasing issue.
    -Wunused-variable
    -std=c++0x
    -fno-strict-aliasing
    #-DENABLE_OGL_DEBUG # FIXME remove me when code is ready
    )

set(OptimizationFlags
    -O2
    -DNDEBUG
    )

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(${CommonFlags} -DENABLE_OGL_DEBUG -g -Wall)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
    add_definitions(${CommonFlags} ${OptimizationFlags} -g -W)
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
    add_definitions(${CommonFlags} ${OptimizationFlags} -W)
endif(CMAKE_BUILD_TYPE STREQUAL Release)

if(XDG_STD)
    add_definitions(-DXDG_STD)
endif(XDG_STD)

# Select the EGL API
if(EGL_API AND EGL_FOUND)
    if (EGL_GL_CONTEXT_SUPPORT)
        add_definitions(-DEGL_API)
    else()
        message(WARNING "Current EGL implementation doesn't support openGL context. Fallback to standard GLX.")
    endif()
endif()

set(GSdxSources
    GLLoader.cpp
    GPU.cpp
    GPUDrawScanline.cpp
    GPUDrawScanlineCodeGenerator.cpp
    GPULocalMemory.cpp
    GPURenderer.cpp
    GPURendererSW.cpp
    GPUSetupPrimCodeGenerator.cpp
    GPUState.cpp
    GS.cpp
    GSAlignedClass.cpp
    GSBlock.cpp
    GSCapture.cpp
    GSClut.cpp
    GSCodeBuffer.cpp
    GSCrc.cpp
    GSDevice.cpp
    GSDeviceOGL.cpp
	GSDeviceSW.cpp
    GSDeviceNull.cpp
    GSDirtyRect.cpp
    GSDrawScanline.cpp
    GSDrawScanlineCodeGenerator.cpp
    GSDrawScanlineCodeGenerator.x86.avx.cpp
    GSDrawScanlineCodeGenerator.x64.cpp
    GSDrawScanlineCodeGenerator.x86.cpp
    GSDrawScanlineCodeGenerator.x64.avx.cpp
    GSDump.cpp
    GSFunctionMap.cpp
    GSLinuxDialog.cpp
    GSLocalMemory.cpp
    GSPerfMon.cpp
    GSRasterizer.cpp
    GSRenderer.cpp
    GSRendererHW.cpp
    GSRendererNull.cpp
    GSRendererOGL.cpp
    GSRendererSW.cpp
    GSSetting.cpp
    GSSetupPrimCodeGenerator.cpp
    GSSetupPrimCodeGenerator.x86.avx.cpp
    GSSetupPrimCodeGenerator.x64.avx.cpp
    GSSetupPrimCodeGenerator.x86.cpp
    GSSetupPrimCodeGenerator.x64.cpp
    GSState.cpp
    GSTables.cpp
    GSTexture.cpp
    GSTextureCache.cpp
    GSTextureCacheSW.cpp
    GSTextureCacheOGL.cpp
    GSTextureFXOGL.cpp
    GSTextureOGL.cpp
    GSTextureNull.cpp
	GSTextureSW.cpp
    GSThread.cpp
    GSUtil.cpp
    GSVector.cpp
    GSVertexTrace.cpp
    GSWnd.cpp
    GSWndOGL.cpp
    GSWndEGL.cpp
    GSdx.cpp
    stdafx.cpp
    )

set(GSdxHeaders
    GPU.h
    GPUDrawScanline.h
    GPUDrawScanlineCodeGenerator.h
    GPUDrawingEnvironment.h
    GPULocalMemory.h
    GPURenderer.h
    GPURendererSW.h
    GPUScanlineEnvironment.h
    GPUSetupPrimCodeGenerator.h
    GPUState.h
    GPUVertex.h
    GS.h
    GSAlignedClass.h
    GSBlock.h
    GSCapture.h
    GSClut.h
    GSCodeBuffer.h
    GSCrc.h
    GSDevice.h
    GSDeviceOGL.h
    GSDeviceNull.h
    GSDirtyRect.h
    GSDrawScanline.h
    GSDrawScanlineCodeGenerator.h
    GSDrawingContext.h
    GSDrawingEnvironment.h
    GSDump.h
    GSFunctionMap.h
    GSLinuxLogo.h
    GSLocalMemory.h
    GSPerfMon.h
    GSRasterizer.h
    GSRenderer.h
    GSRendererNull.h
    GSRendererSW.h
    GSRendererHW.h
    GSRendererOGL.h
    GSScanlineEnvironment.h
    GSSetting.h
    GSSetupPrimCodeGenerator.h
    GSState.h
    GSTables.h
    GSTexture.h
    GSTextureCache.h
    GSTextureCacheSW.h
    GSTextureCacheOGL.h
    GSTextureNull.h
    GSThread.h
    GSUtil.h
    GSVector.h
    GSVertex.h
    GSVertexHW.h
    GSVertexList.h
    GSVertexSW.h
    GSVertexTrace.h
    GSWnd.h
    GSWndOGL.h
    GSWndEGL.h
    GSdx.h
    stdafx.h
    xbyak/xbyak.h
    xbyak/xbyak_bin2hex.h
    xbyak/xbyak_mnemonic.h
    xbyak/xbyak_util.h
    )

include_directories(.)

add_library(${Output} SHARED ${GSdxSources} ${GSdxHeaders})

target_link_libraries(${Output} ${X11_LIBRARIES})
target_link_libraries(${Output} ${OPENGL_LIBRARIES})
if(EGL_API AND EGL_FOUND AND EGL_GL_CONTEXT_SUPPORT)
    target_link_libraries(${Output} ${EGL_LIBRARIES})
endif()

if(Linux)
	target_link_libraries(${Output} ${GTK2_LIBRARIES})
endif(Linux)

if(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
    target_link_libraries(${Output} "${USER_CMAKE_LD_FLAGS}")
endif(NOT USER_CMAKE_LD_FLAGS STREQUAL "")

if(PACKAGE_MODE)
    install(TARGETS ${Output} DESTINATION ${PLUGIN_DIR})

    foreach(glsl IN ITEMS convert.glsl interlace.glsl merge.glsl tfx.glsl shadeboost.glsl)
        install(FILES ${PROJECT_SOURCE_DIR}/plugins/GSdx/res/${glsl} DESTINATION ${GLSL_SHADER_DIR})
    endforeach(glsl IN ITEMS convert.glsl interlace.glsl merge.glsl tfx.glsl shadeboost.glsl)
else(PACKAGE_MODE)
    install(TARGETS ${Output} DESTINATION ${CMAKE_SOURCE_DIR}/bin/plugins)

    foreach(glsl IN ITEMS convert.glsl interlace.glsl merge.glsl tfx.glsl shadeboost.glsl)
        install(FILES ${PROJECT_SOURCE_DIR}/plugins/GSdx/res/${glsl} DESTINATION ${CMAKE_SOURCE_DIR}/bin/plugins)
    endforeach(glsl IN ITEMS convert.glsl interlace.glsl merge.glsl tfx.glsl shadeboost.glsl)
endif(PACKAGE_MODE)

################################### Replay Loader
if(BUILD_REPLAY_LOADERS)
	set(Replay pcsx2_GSReplayLoader)

	add_executable(${Replay} linux_replay.cpp)

    target_link_libraries(${Replay} ${LIBC_LIBRARIES})

	if(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
	    target_link_libraries(${Replay} "${USER_CMAKE_LD_FLAGS}")
	endif(NOT USER_CMAKE_LD_FLAGS STREQUAL "")

	if(PACKAGE_MODE)
	    install(TARGETS ${Replay} DESTINATION bin)
	else(PACKAGE_MODE)
	    install(TARGETS ${Replay} DESTINATION ${CMAKE_SOURCE_DIR}/bin)
	endif(PACKAGE_MODE)
endif(BUILD_REPLAY_LOADERS)
