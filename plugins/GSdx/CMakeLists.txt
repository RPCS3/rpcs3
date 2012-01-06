# Check that people use the good file
if(NOT TOP_CMAKE_WAS_SOURCED)
    message(FATAL_ERROR "
    You did not 'cmake' the good CMakeLists.txt file. Use the one in the top dir.
    It is advice to delete all wrongly generated cmake stuff => CMakeFiles & CMakeCache.txt")
endif(NOT TOP_CMAKE_WAS_SOURCED)


# plugin name
set(Output GSdx-0.1.16)
set(Replay pcsx2_GSReplayLoader)
set(Static GSdx-static)

set(CommonFlags
    -D_LINUX
    -fno-operator-names
    -mpreferred-stack-boundary=2
    -mfpmath=sse
    #-Wstrict-aliasing # Allow to track strict aliasing issue.
    -Wunused-variable
    -std=c++0x
    -fno-strict-aliasing
    -DOGL_DEBUG # FIXME remove me when code is ready
	-DAMD_DRIVER_WORKAROUND
    )

set(OptimizationFlags
    -O2
    -DNDEBUG
    )

if(projectSDL)
    set(SDLFlags -DENABLE_SDL_DEV)
else(projectSDL)
    set(SDLFlags "")
endif(projectSDL)

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(${CommonFlags} ${SDLFlags} -DOGL_DEBUG -g -Wall)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
    add_definitions(${CommonFlags} ${SDLFlags} ${OptimizationFlags} -g -W)
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
    add_definitions(${CommonFlags} ${SDLFlags} ${OptimizationFlags} -W)
endif(CMAKE_BUILD_TYPE STREQUAL Release)

set(GSdxSources
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
    GSDeviceSDL.cpp
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
    GSVertexTrace.x64.avx.cpp
    GSVertexTrace.x86.cpp
    GSVertexTrace.x86.avx.cpp
    GSVertexTrace.x64.cpp
    GSWnd.cpp
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
    GSdx.h
    stdafx.h
    xbyak/xbyak.h
    xbyak/xbyak_bin2hex.h
    xbyak/xbyak_mnemonic.h
    xbyak/xbyak_util.h
    )

include_directories(.)

add_library(${Output} SHARED ${GSdxSources} ${GSdxHeaders})

add_library(${Static} STATIC ${GSdxSources} ${GSdxHeaders})

add_executable(${Replay} linux_replay.cpp)

target_link_libraries(${Output} ${X11_LIBRARIES})
target_link_libraries(${Output} ${GLEW_LIBRARY})
target_link_libraries(${Output} ${OPENGL_LIBRARIES})

if(projectSDL)
    target_link_libraries(${Output} ${SDL_LIBRARY})
endif(projectSDL)

target_link_libraries(${Replay} ${OPENGL_LIBRARIES})
target_link_libraries(${Replay} ${X11_LIBRARIES})
target_link_libraries(${Replay} ${GLEW_LIBRARY})
target_link_libraries(${Replay} ${GTK2_LIBRARIES})
target_link_libraries(${Replay} ${Static})


if(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
    target_link_libraries(${Output} "${USER_CMAKE_LD_FLAGS}")
endif(NOT USER_CMAKE_LD_FLAGS STREQUAL "")

if(PACKAGE_MODE)
    install(TARGETS ${Replay} DESTINATION bin)

    install(TARGETS ${Output} DESTINATION ${PLUGIN_DIR})

    install(FILES ${PROJECT_SOURCE_DIR}/plugins/GSdx/res/convert.glsl DESTINATION ${PLUGIN_DIR})
    install(FILES ${PROJECT_SOURCE_DIR}/plugins/GSdx/res/interlace.glsl DESTINATION ${PLUGIN_DIR})
    install(FILES ${PROJECT_SOURCE_DIR}/plugins/GSdx/res/merge.glsl DESTINATION ${PLUGIN_DIR})
    install(FILES ${PROJECT_SOURCE_DIR}/plugins/GSdx/res/tfx.glsl DESTINATION ${PLUGIN_DIR})
else(PACKAGE_MODE)
    install(TARGETS ${Replay} DESTINATION ${CMAKE_SOURCE_DIR}/bin)

    install(TARGETS ${Output} DESTINATION ${CMAKE_SOURCE_DIR}/bin/plugins)

    install(FILES ${PROJECT_SOURCE_DIR}/plugins/GSdx/res/convert.glsl DESTINATION ${CMAKE_SOURCE_DIR}/bin/plugins)
    install(FILES ${PROJECT_SOURCE_DIR}/plugins/GSdx/res/interlace.glsl DESTINATION ${CMAKE_SOURCE_DIR}/bin/plugins)
    install(FILES ${PROJECT_SOURCE_DIR}/plugins/GSdx/res/merge.glsl DESTINATION ${CMAKE_SOURCE_DIR}/bin/plugins)
    install(FILES ${PROJECT_SOURCE_DIR}/plugins/GSdx/res/tfx.glsl DESTINATION ${CMAKE_SOURCE_DIR}/bin/plugins)
endif(PACKAGE_MODE)
