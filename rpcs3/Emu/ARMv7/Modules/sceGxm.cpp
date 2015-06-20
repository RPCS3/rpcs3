#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceGxm.h"

s32 sceGxmInitialize(vm::ptr<const SceGxmInitializeParams> params)
{
	throw __FUNCTION__;
}

s32 sceGxmTerminate()
{
	throw __FUNCTION__;
}

vm::ptr<volatile u32> sceGxmGetNotificationRegion()
{
	throw __FUNCTION__;
}

s32 sceGxmNotificationWait(vm::ptr<const SceGxmNotification> notification)
{
	throw __FUNCTION__;
}

s32 sceGxmMapMemory(vm::ptr<void> base, u32 size, SceGxmMemoryAttribFlags attr)
{
	throw __FUNCTION__;
}

s32 sceGxmUnmapMemory(vm::ptr<void> base)
{
	throw __FUNCTION__;
}

s32 sceGxmMapVertexUsseMemory(vm::ptr<void> base, u32 size, vm::ptr<u32> offset)
{
	throw __FUNCTION__;
}

s32 sceGxmUnmapVertexUsseMemory(vm::ptr<void> base)
{
	throw __FUNCTION__;
}

s32 sceGxmMapFragmentUsseMemory(vm::ptr<void> base, u32 size, vm::ptr<u32> offset)
{
	throw __FUNCTION__;
}

s32 sceGxmUnmapFragmentUsseMemory(vm::ptr<void> base)
{
	throw __FUNCTION__;
}

s32 sceGxmDisplayQueueAddEntry(vm::ptr<SceGxmSyncObject> oldBuffer, vm::ptr<SceGxmSyncObject> newBuffer, vm::ptr<const void> callbackData)
{
	throw __FUNCTION__;
}

s32 sceGxmDisplayQueueFinish()
{
	throw __FUNCTION__;
}

s32 sceGxmSyncObjectCreate(vm::pptr<SceGxmSyncObject> syncObject)
{
	throw __FUNCTION__;
}

s32 sceGxmSyncObjectDestroy(vm::ptr<SceGxmSyncObject> syncObject)
{
	throw __FUNCTION__;
}


s32 sceGxmCreateContext(vm::ptr<const SceGxmContextParams> params, vm::pptr<SceGxmContext> context)
{
	throw __FUNCTION__;
}

s32 sceGxmDestroyContext(vm::ptr<SceGxmContext> context)
{
	throw __FUNCTION__;
}

void sceGxmSetValidationEnable(vm::ptr<SceGxmContext> context, bool enable)
{
	throw __FUNCTION__;
}


void sceGxmSetVertexProgram(vm::ptr<SceGxmContext> context, vm::ptr<const SceGxmVertexProgram> vertexProgram)
{
	throw __FUNCTION__;
}

void sceGxmSetFragmentProgram(vm::ptr<SceGxmContext> context, vm::ptr<const SceGxmFragmentProgram> fragmentProgram)
{
	throw __FUNCTION__;
}

s32 sceGxmReserveVertexDefaultUniformBuffer(vm::ptr<SceGxmContext> context, vm::pptr<void> uniformBuffer)
{
	throw __FUNCTION__;
}

s32 sceGxmReserveFragmentDefaultUniformBuffer(vm::ptr<SceGxmContext> context, vm::pptr<void> uniformBuffer)
{
	throw __FUNCTION__;
}

s32 sceGxmSetVertexStream(vm::ptr<SceGxmContext> context, u32 streamIndex, vm::ptr<const void> streamData)
{
	throw __FUNCTION__;
}

s32 sceGxmSetVertexTexture(vm::ptr<SceGxmContext> context, u32 textureIndex, vm::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmSetFragmentTexture(vm::ptr<SceGxmContext> context, u32 textureIndex, vm::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmSetVertexUniformBuffer(vm::ptr<SceGxmContext> context, u32 bufferIndex, vm::ptr<const void> bufferData)
{
	throw __FUNCTION__;
}

s32 sceGxmSetFragmentUniformBuffer(vm::ptr<SceGxmContext> context, u32 bufferIndex, vm::ptr<const void> bufferData)
{
	throw __FUNCTION__;
}

s32 sceGxmSetAuxiliarySurface(vm::ptr<SceGxmContext> context, u32 surfaceIndex, vm::ptr<const SceGxmAuxiliarySurface> surface)
{
	throw __FUNCTION__;
}


void sceGxmSetPrecomputedFragmentState(vm::ptr<SceGxmContext> context, vm::ptr<const SceGxmPrecomputedFragmentState> precomputedState)
{
	throw __FUNCTION__;
}

void sceGxmSetPrecomputedVertexState(vm::ptr<SceGxmContext> context, vm::ptr<const SceGxmPrecomputedVertexState> precomputedState)
{
	throw __FUNCTION__;
}

s32 sceGxmDrawPrecomputed(vm::ptr<SceGxmContext> context, vm::ptr<const SceGxmPrecomputedDraw> precomputedDraw)
{
	throw __FUNCTION__;
}

s32 sceGxmDraw(vm::ptr<SceGxmContext> context, SceGxmPrimitiveType primType, SceGxmIndexFormat indexType, vm::ptr<const void> indexData, u32 indexCount)
{
	throw __FUNCTION__;
}

s32 sceGxmDrawInstanced(vm::ptr<SceGxmContext> context, SceGxmPrimitiveType primType, SceGxmIndexFormat indexType, vm::ptr<const void> indexData, u32 indexCount, u32 indexWrap)
{
	throw __FUNCTION__;
}

s32 sceGxmSetVisibilityBuffer(vm::ptr<SceGxmContext> context, vm::ptr<void> bufferBase, u32 stridePerCore)
{
	throw __FUNCTION__;
}

s32 sceGxmBeginScene(vm::ptr<SceGxmContext> context, u32 flags, vm::ptr<const SceGxmRenderTarget> renderTarget, vm::ptr<const SceGxmValidRegion> validRegion, vm::ptr<SceGxmSyncObject> vertexSyncObject, vm::ptr<SceGxmSyncObject> fragmentSyncObject, vm::ptr<const SceGxmColorSurface> colorSurface, vm::ptr<const SceGxmDepthStencilSurface> depthStencil)
{
	throw __FUNCTION__;
}

s32 sceGxmMidSceneFlush(vm::ptr<SceGxmContext> context, u32 flags, vm::ptr<SceGxmSyncObject> vertexSyncObject, vm::ptr<const SceGxmNotification> vertexNotification)
{
	throw __FUNCTION__;
}

s32 sceGxmEndScene(vm::ptr<SceGxmContext> context, vm::ptr<const SceGxmNotification> vertexNotification, vm::ptr<const SceGxmNotification> fragmentNotification)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontDepthFunc(vm::ptr<SceGxmContext> context, SceGxmDepthFunc depthFunc)
{
	throw __FUNCTION__;
}

void sceGxmSetBackDepthFunc(vm::ptr<SceGxmContext> context, SceGxmDepthFunc depthFunc)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontFragmentProgramEnable(vm::ptr<SceGxmContext> context, SceGxmFragmentProgramMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetBackFragmentProgramEnable(vm::ptr<SceGxmContext> context, SceGxmFragmentProgramMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontDepthWriteEnable(vm::ptr<SceGxmContext> context, SceGxmDepthWriteMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetBackDepthWriteEnable(vm::ptr<SceGxmContext> context, SceGxmDepthWriteMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontLineFillLastPixelEnable(vm::ptr<SceGxmContext> context, SceGxmLineFillLastPixelMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetBackLineFillLastPixelEnable(vm::ptr<SceGxmContext> context, SceGxmLineFillLastPixelMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontStencilRef(vm::ptr<SceGxmContext> context, u32 sref)
{
	throw __FUNCTION__;
}

void sceGxmSetBackStencilRef(vm::ptr<SceGxmContext> context, u32 sref)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontPointLineWidth(vm::ptr<SceGxmContext> context, u32 width)
{
	throw __FUNCTION__;
}

void sceGxmSetBackPointLineWidth(vm::ptr<SceGxmContext> context, u32 width)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontPolygonMode(vm::ptr<SceGxmContext> context, SceGxmPolygonMode mode)
{
	throw __FUNCTION__;
}

void sceGxmSetBackPolygonMode(vm::ptr<SceGxmContext> context, SceGxmPolygonMode mode)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontStencilFunc(vm::ptr<SceGxmContext> context, SceGxmStencilFunc func, SceGxmStencilOp stencilFail, SceGxmStencilOp depthFail, SceGxmStencilOp depthPass, u8 compareMask, u8 writeMask)
{
	throw __FUNCTION__;
}


void sceGxmSetBackStencilFunc(vm::ptr<SceGxmContext> context, SceGxmStencilFunc func, SceGxmStencilOp stencilFail, SceGxmStencilOp depthFail, SceGxmStencilOp depthPass, u8 compareMask, u8 writeMask)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontDepthBias(vm::ptr<SceGxmContext> context, s32 factor, s32 units)
{
	throw __FUNCTION__;
}

void sceGxmSetBackDepthBias(vm::ptr<SceGxmContext> context, s32 factor, s32 units)
{
	throw __FUNCTION__;
}

void sceGxmSetTwoSidedEnable(vm::ptr<SceGxmContext> context, SceGxmTwoSidedMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetViewport(vm::ptr<SceGxmContext> context, float xOffset, float xScale, float yOffset, float yScale, float zOffset, float zScale)
{
	throw __FUNCTION__;
}

void sceGxmSetWClampValue(vm::ptr<SceGxmContext> context, float clampValue)
{
	throw __FUNCTION__;
}

void sceGxmSetWClampEnable(vm::ptr<SceGxmContext> context, SceGxmWClampMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetRegionClip(vm::ptr<SceGxmContext> context, SceGxmRegionClipMode mode, u32 xMin, u32 yMin, u32 xMax, u32 yMax)
{
	throw __FUNCTION__;
}

void sceGxmSetCullMode(vm::ptr<SceGxmContext> context, SceGxmCullMode mode)
{
	throw __FUNCTION__;
}

void sceGxmSetViewportEnable(vm::ptr<SceGxmContext> context, SceGxmViewportMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetWBufferEnable(vm::ptr<SceGxmContext> context, SceGxmWBufferMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontVisibilityTestIndex(vm::ptr<SceGxmContext> context, u32 index)
{
	throw __FUNCTION__;
}

void sceGxmSetBackVisibilityTestIndex(vm::ptr<SceGxmContext> context, u32 index)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontVisibilityTestOp(vm::ptr<SceGxmContext> context, SceGxmVisibilityTestOp op)
{
	throw __FUNCTION__;
}

void sceGxmSetBackVisibilityTestOp(vm::ptr<SceGxmContext> context, SceGxmVisibilityTestOp op)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontVisibilityTestEnable(vm::ptr<SceGxmContext> context, SceGxmVisibilityTestMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetBackVisibilityTestEnable(vm::ptr<SceGxmContext> context, SceGxmVisibilityTestMode enable)
{
	throw __FUNCTION__;
}

void sceGxmFinish(vm::ptr<SceGxmContext> context)
{
	throw __FUNCTION__;
}

s32 sceGxmPushUserMarker(vm::ptr<SceGxmContext> context, vm::ptr<const char> tag)
{
	throw __FUNCTION__;
}

s32 sceGxmPopUserMarker(vm::ptr<SceGxmContext> context)
{
	throw __FUNCTION__;
}

s32 sceGxmSetUserMarker(vm::ptr<SceGxmContext> context, vm::ptr<const char> tag)
{
	throw __FUNCTION__;
}

s32 sceGxmPadHeartbeat(vm::ptr<const SceGxmColorSurface> displaySurface, vm::ptr<SceGxmSyncObject> displaySyncObject)
{
	throw __FUNCTION__;
}

s32 sceGxmPadTriggerGpuPaTrace()
{
	throw __FUNCTION__;
}

s32 sceGxmColorSurfaceInit(vm::ptr<SceGxmColorSurface> surface, SceGxmColorFormat colorFormat, SceGxmColorSurfaceType surfaceType, SceGxmColorSurfaceScaleMode scaleMode, SceGxmOutputRegisterSize outputRegisterSize, u32 width, u32 height, u32 strideInPixels, vm::ptr<void> data)
{
	throw __FUNCTION__;
}

s32 sceGxmColorSurfaceInitDisabled(vm::ptr<SceGxmColorSurface> surface)
{
	throw __FUNCTION__;
}

bool sceGxmColorSurfaceIsEnabled(vm::ptr<const SceGxmColorSurface> surface)
{
	throw __FUNCTION__;
}

void sceGxmColorSurfaceGetClip(vm::ptr<const SceGxmColorSurface> surface, vm::ptr<u32> xMin, vm::ptr<u32> yMin, vm::ptr<u32> xMax, vm::ptr<u32> yMax)
{
	throw __FUNCTION__;
}

void sceGxmColorSurfaceSetClip(vm::ptr<SceGxmColorSurface> surface, u32 xMin, u32 yMin, u32 xMax, u32 yMax)
{
	throw __FUNCTION__;
}

SceGxmColorSurfaceScaleMode sceGxmColorSurfaceGetScaleMode(vm::ptr<const SceGxmColorSurface> surface)
{
	throw __FUNCTION__;
}

void sceGxmColorSurfaceSetScaleMode(vm::ptr<SceGxmColorSurface> surface, SceGxmColorSurfaceScaleMode scaleMode)
{
	throw __FUNCTION__;
}

vm::ptr<void> sceGxmColorSurfaceGetData(vm::ptr<const SceGxmColorSurface> surface)
{
	throw __FUNCTION__;
}

s32 sceGxmColorSurfaceSetData(vm::ptr<SceGxmColorSurface> surface, vm::ptr<void> data)
{
	throw __FUNCTION__;
}

SceGxmColorFormat sceGxmColorSurfaceGetFormat(vm::ptr<const SceGxmColorSurface> surface)
{
	throw __FUNCTION__;
}

s32 sceGxmColorSurfaceSetFormat(vm::ptr<SceGxmColorSurface> surface, SceGxmColorFormat format)
{
	throw __FUNCTION__;
}

SceGxmColorSurfaceType sceGxmColorSurfaceGetType(vm::ptr<const SceGxmColorSurface> surface)
{
	throw __FUNCTION__;
}

u32 sceGxmColorSurfaceGetStrideInPixels(vm::ptr<const SceGxmColorSurface> surface)
{
	throw __FUNCTION__;
}

s32 sceGxmDepthStencilSurfaceInit(vm::ptr<SceGxmDepthStencilSurface> surface, SceGxmDepthStencilFormat depthStencilFormat, SceGxmDepthStencilSurfaceType surfaceType, u32 strideInSamples, vm::ptr<void> depthData, vm::ptr<void> stencilData)
{
	throw __FUNCTION__;
}

s32 sceGxmDepthStencilSurfaceInitDisabled(vm::ptr<SceGxmDepthStencilSurface> surface)
{
	throw __FUNCTION__;
}

//float sceGxmDepthStencilSurfaceGetBackgroundDepth(vm::ptr<const SceGxmDepthStencilSurface> surface)
//{
//	throw __FUNCTION__;
//}

//void sceGxmDepthStencilSurfaceSetBackgroundDepth(vm::ptr<SceGxmDepthStencilSurface> surface, float backgroundDepth)
//{
//	throw __FUNCTION__;
//}

u8 sceGxmDepthStencilSurfaceGetBackgroundStencil(vm::ptr<const SceGxmDepthStencilSurface> surface)
{
	throw __FUNCTION__;
}

void sceGxmDepthStencilSurfaceSetBackgroundStencil(vm::ptr<SceGxmDepthStencilSurface> surface, u8 backgroundStencil)
{
	throw __FUNCTION__;
}

bool sceGxmDepthStencilSurfaceIsEnabled(vm::ptr<const SceGxmDepthStencilSurface> surface)
{
	throw __FUNCTION__;
}

void sceGxmDepthStencilSurfaceSetForceLoadMode(vm::ptr<SceGxmDepthStencilSurface> surface, SceGxmDepthStencilForceLoadMode forceLoad)
{
	throw __FUNCTION__;
}

SceGxmDepthStencilForceLoadMode sceGxmDepthStencilSurfaceGetForceLoadMode(vm::ptr<const SceGxmDepthStencilSurface> surface)
{
	throw __FUNCTION__;
}

void sceGxmDepthStencilSurfaceSetForceStoreMode(vm::ptr<SceGxmDepthStencilSurface> surface, SceGxmDepthStencilForceStoreMode forceStore)
{
	throw __FUNCTION__;
}

SceGxmDepthStencilForceStoreMode sceGxmDepthStencilSurfaceGetForceStoreMode(vm::ptr<const SceGxmDepthStencilSurface> surface)
{
	throw __FUNCTION__;
}

SceGxmColorSurfaceGammaMode sceGxmColorSurfaceGetGammaMode(vm::ptr<const SceGxmColorSurface> surface)
{
	throw __FUNCTION__;
}

s32 sceGxmColorSurfaceSetGammaMode(vm::ptr<SceGxmColorSurface> surface, SceGxmColorSurfaceGammaMode gammaMode)
{
	throw __FUNCTION__;
}

SceGxmColorSurfaceDitherMode sceGxmColorSurfaceGetDitherMode(vm::ptr<const SceGxmColorSurface> surface)
{
	throw __FUNCTION__;
}

s32 sceGxmColorSurfaceSetDitherMode(vm::ptr<SceGxmColorSurface> surface, SceGxmColorSurfaceDitherMode ditherMode)
{
	throw __FUNCTION__;
}

SceGxmDepthStencilFormat sceGxmDepthStencilSurfaceGetFormat(vm::ptr<const SceGxmDepthStencilSurface> surface)
{
	throw __FUNCTION__;
}

u32 sceGxmDepthStencilSurfaceGetStrideInSamples(vm::ptr<const SceGxmDepthStencilSurface> surface)
{
	throw __FUNCTION__;
}


s32 sceGxmProgramCheck(vm::ptr<const SceGxmProgram> program)
{
	throw __FUNCTION__;
}

u32 sceGxmProgramGetSize(vm::ptr<const SceGxmProgram> program)
{
	throw __FUNCTION__;
}

SceGxmProgramType sceGxmProgramGetType(vm::ptr<const SceGxmProgram> program)
{
	throw __FUNCTION__;
}

bool sceGxmProgramIsDiscardUsed(vm::ptr<const SceGxmProgram> program)
{
	throw __FUNCTION__;
}

bool sceGxmProgramIsDepthReplaceUsed(vm::ptr<const SceGxmProgram> program)
{
	throw __FUNCTION__;
}

bool sceGxmProgramIsSpriteCoordUsed(vm::ptr<const SceGxmProgram> program)
{
	throw __FUNCTION__;
}

u32 sceGxmProgramGetDefaultUniformBufferSize(vm::ptr<const SceGxmProgram> program)
{
	throw __FUNCTION__;
}

u32 sceGxmProgramGetParameterCount(vm::ptr<const SceGxmProgram> program)
{
	throw __FUNCTION__;
}

vm::ptr<const SceGxmProgramParameter> sceGxmProgramGetParameter(vm::ptr<const SceGxmProgram> program, u32 index)
{
	throw __FUNCTION__;
}

vm::ptr<const SceGxmProgramParameter> sceGxmProgramFindParameterByName(vm::ptr<const SceGxmProgram> program, vm::ptr<const char> name)
{
	throw __FUNCTION__;
}

vm::ptr<const SceGxmProgramParameter> sceGxmProgramFindParameterBySemantic(vm::ptr<const SceGxmProgram> program, SceGxmParameterSemantic semantic, u32 index)
{
	throw __FUNCTION__;
}

u32 sceGxmProgramParameterGetIndex(vm::ptr<const SceGxmProgram> program, vm::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

SceGxmParameterCategory sceGxmProgramParameterGetCategory(vm::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

vm::ptr<const char> sceGxmProgramParameterGetName(vm::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

SceGxmParameterSemantic sceGxmProgramParameterGetSemantic(vm::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

u32 sceGxmProgramParameterGetSemanticIndex(vm::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

SceGxmParameterType sceGxmProgramParameterGetType(vm::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

u32 sceGxmProgramParameterGetComponentCount(vm::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

u32 sceGxmProgramParameterGetArraySize(vm::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

u32 sceGxmProgramParameterGetResourceIndex(vm::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

u32 sceGxmProgramParameterGetContainerIndex(vm::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

bool sceGxmProgramParameterIsSamplerCube(vm::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

vm::ptr<const SceGxmProgram> sceGxmFragmentProgramGetProgram(vm::ptr<const SceGxmFragmentProgram> fragmentProgram)
{
	throw __FUNCTION__;
}

vm::ptr<const SceGxmProgram> sceGxmVertexProgramGetProgram(vm::ptr<const SceGxmVertexProgram> vertexProgram)
{
	throw __FUNCTION__;
}


s32 sceGxmShaderPatcherCreate(vm::ptr<const SceGxmShaderPatcherParams> params, vm::pptr<SceGxmShaderPatcher> shaderPatcher)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherSetUserData(vm::ptr<SceGxmShaderPatcher> shaderPatcher, vm::ptr<void> userData)
{
	throw __FUNCTION__;
}

vm::ptr<void> sceGxmShaderPatcherGetUserData(vm::ptr<SceGxmShaderPatcher> shaderPatcher)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherDestroy(vm::ptr<SceGxmShaderPatcher> shaderPatcher)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherRegisterProgram(vm::ptr<SceGxmShaderPatcher> shaderPatcher, vm::ptr<const SceGxmProgram> programHeader, vm::pptr<SceGxmRegisteredProgram> programId)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherUnregisterProgram(vm::ptr<SceGxmShaderPatcher> shaderPatcher, vm::ptr<SceGxmRegisteredProgram> programId)
{
	throw __FUNCTION__;
}

vm::ptr<const SceGxmProgram> sceGxmShaderPatcherGetProgramFromId(vm::ptr<SceGxmRegisteredProgram> programId)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherSetAuxiliarySurface(vm::ptr<SceGxmShaderPatcher> shaderPatcher, u32 auxSurfaceIndex, vm::ptr<const SceGxmAuxiliarySurface> auxSurface)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherCreateVertexProgram(vm::ptr<SceGxmShaderPatcher> shaderPatcher, vm::ptr<SceGxmRegisteredProgram> programId, vm::ptr<const SceGxmVertexAttribute> attributes, u32 attributeCount, vm::ptr<const SceGxmVertexStream> streams, u32 streamCount, vm::pptr<SceGxmVertexProgram> vertexProgram)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherCreateFragmentProgram(vm::ptr<SceGxmShaderPatcher> shaderPatcher, vm::ptr<SceGxmRegisteredProgram> programId, SceGxmOutputRegisterFormat outputFormat, SceGxmMultisampleMode multisampleMode, vm::ptr<const SceGxmBlendInfo> blendInfo, vm::ptr<const SceGxmProgram> vertexProgram, vm::pptr<SceGxmFragmentProgram> fragmentProgram)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherAddRefVertexProgram(vm::ptr<SceGxmShaderPatcher> shaderPatcher, vm::ptr<SceGxmVertexProgram> vertexProgram)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherAddRefFragmentProgram(vm::ptr<SceGxmShaderPatcher> shaderPatcher, vm::ptr<SceGxmFragmentProgram> fragmentProgram)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherReleaseVertexProgram(vm::ptr<SceGxmShaderPatcher> shaderPatcher, vm::ptr<SceGxmVertexProgram> vertexProgram)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherReleaseFragmentProgram(vm::ptr<SceGxmShaderPatcher> shaderPatcher, vm::ptr<SceGxmFragmentProgram> fragmentProgram)
{
	throw __FUNCTION__;
}

u32 sceGxmShaderPatcherGetHostMemAllocated(vm::ptr<const SceGxmShaderPatcher> shaderPatcher)
{
	throw __FUNCTION__;
}

u32 sceGxmShaderPatcherGetBufferMemAllocated(vm::ptr<const SceGxmShaderPatcher> shaderPatcher)
{
	throw __FUNCTION__;
}

u32 sceGxmShaderPatcherGetVertexUsseMemAllocated(vm::ptr<const SceGxmShaderPatcher> shaderPatcher)
{
	throw __FUNCTION__;
}

u32 sceGxmShaderPatcherGetFragmentUsseMemAllocated(vm::ptr<const SceGxmShaderPatcher> shaderPatcher)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureInitSwizzled(vm::ptr<SceGxmTexture> texture, vm::ptr<const void> data, SceGxmTextureFormat texFormat, u32 width, u32 height, u32 mipCount)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureInitLinear(vm::ptr<SceGxmTexture> texture, vm::ptr<const void> data, SceGxmTextureFormat texFormat, u32 width, u32 height, u32 mipCount)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureInitLinearStrided(vm::ptr<SceGxmTexture> texture, vm::ptr<const void> data, SceGxmTextureFormat texFormat, u32 width, u32 height, u32 byteStride)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureInitTiled(vm::ptr<SceGxmTexture> texture, vm::ptr<const void> data, SceGxmTextureFormat texFormat, u32 width, u32 height, u32 mipCount)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureInitCube(vm::ptr<SceGxmTexture> texture, vm::ptr<const void> data, SceGxmTextureFormat texFormat, u32 width, u32 height, u32 mipCount)
{
	throw __FUNCTION__;
}

SceGxmTextureType sceGxmTextureGetType(vm::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetMinFilter(vm::ptr<SceGxmTexture> texture, SceGxmTextureFilter minFilter)
{
	throw __FUNCTION__;
}

SceGxmTextureFilter sceGxmTextureGetMinFilter(vm::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetMagFilter(vm::ptr<SceGxmTexture> texture, SceGxmTextureFilter magFilter)
{
	throw __FUNCTION__;
}

SceGxmTextureFilter sceGxmTextureGetMagFilter(vm::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetMipFilter(vm::ptr<SceGxmTexture> texture, SceGxmTextureMipFilter mipFilter)
{
	throw __FUNCTION__;
}

SceGxmTextureMipFilter sceGxmTextureGetMipFilter(vm::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetAnisoMode(vm::ptr<SceGxmTexture> texture, SceGxmTextureAnisoMode anisoMode)
{
	throw __FUNCTION__;
}

SceGxmTextureAnisoMode sceGxmTextureGetAnisoMode(vm::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetUAddrMode(vm::ptr<SceGxmTexture> texture, SceGxmTextureAddrMode addrMode)
{
	throw __FUNCTION__;
}

SceGxmTextureAddrMode sceGxmTextureGetUAddrMode(vm::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetVAddrMode(vm::ptr<SceGxmTexture> texture, SceGxmTextureAddrMode addrMode)
{
	throw __FUNCTION__;
}

SceGxmTextureAddrMode sceGxmTextureGetVAddrMode(vm::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetFormat(vm::ptr<SceGxmTexture> texture, SceGxmTextureFormat texFormat)
{
	throw __FUNCTION__;
}

SceGxmTextureFormat sceGxmTextureGetFormat(vm::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetLodBias(vm::ptr<SceGxmTexture> texture, u32 bias)
{
	throw __FUNCTION__;
}

u32 sceGxmTextureGetLodBias(vm::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetStride(vm::ptr<SceGxmTexture> texture, u32 byteStride)
{
	throw __FUNCTION__;
}

u32 sceGxmTextureGetStride(vm::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetWidth(vm::ptr<SceGxmTexture> texture, u32 width)
{
	throw __FUNCTION__;
}

u32 sceGxmTextureGetWidth(vm::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetHeight(vm::ptr<SceGxmTexture> texture, u32 height)
{
	throw __FUNCTION__;
}

u32 sceGxmTextureGetHeight(vm::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetData(vm::ptr<SceGxmTexture> texture, vm::ptr<const void> data)
{
	throw __FUNCTION__;
}

vm::ptr<void> sceGxmTextureGetData(vm::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetMipmapCount(vm::ptr<SceGxmTexture> texture, u32 mipCount)
{
	throw __FUNCTION__;
}

u32 sceGxmTextureGetMipmapCount(vm::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetPalette(vm::ptr<SceGxmTexture> texture, vm::ptr<const void> paletteData)
{
	throw __FUNCTION__;
}

vm::ptr<void> sceGxmTextureGetPalette(vm::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

SceGxmTextureGammaMode sceGxmTextureGetGammaMode(vm::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetGammaMode(vm::ptr<SceGxmTexture> texture, SceGxmTextureGammaMode gammaMode)
{
	throw __FUNCTION__;
}

u32 sceGxmGetPrecomputedVertexStateSize(vm::ptr<const SceGxmVertexProgram> vertexProgram)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedVertexStateInit(vm::ptr<SceGxmPrecomputedVertexState> precomputedState, vm::ptr<const SceGxmVertexProgram> vertexProgram, vm::ptr<void> memBlock)
{
	throw __FUNCTION__;
}

void sceGxmPrecomputedVertexStateSetDefaultUniformBuffer(vm::ptr<SceGxmPrecomputedVertexState> precomputedState, vm::ptr<void> defaultBuffer)
{
	throw __FUNCTION__;
}

vm::ptr<void> sceGxmPrecomputedVertexStateGetDefaultUniformBuffer(vm::ptr<const SceGxmPrecomputedVertexState> precomputedState)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedVertexStateSetAllTextures(vm::ptr<SceGxmPrecomputedVertexState> precomputedState, vm::ptr<const SceGxmTexture> textures)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedVertexStateSetTexture(vm::ptr<SceGxmPrecomputedVertexState> precomputedState, u32 textureIndex, vm::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedVertexStateSetAllUniformBuffers(vm::ptr<SceGxmPrecomputedVertexState> precomputedState, vm::ptr<const vm::ptr<const void>> bufferDataArray)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedVertexStateSetUniformBuffer(vm::ptr<SceGxmPrecomputedVertexState> precomputedState, u32 bufferIndex, vm::ptr<const void> bufferData)
{
	throw __FUNCTION__;
}

u32 sceGxmGetPrecomputedFragmentStateSize(vm::ptr<const SceGxmFragmentProgram> fragmentProgram)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedFragmentStateInit(vm::ptr<SceGxmPrecomputedFragmentState> precomputedState, vm::ptr<const SceGxmFragmentProgram> fragmentProgram, vm::ptr<void> memBlock)
{
	throw __FUNCTION__;
}

void sceGxmPrecomputedFragmentStateSetDefaultUniformBuffer(vm::ptr<SceGxmPrecomputedFragmentState> precomputedState, vm::ptr<void> defaultBuffer)
{
	throw __FUNCTION__;
}

vm::ptr<void> sceGxmPrecomputedFragmentStateGetDefaultUniformBuffer(vm::ptr<const SceGxmPrecomputedFragmentState> precomputedState)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedFragmentStateSetAllTextures(vm::ptr<SceGxmPrecomputedFragmentState> precomputedState, vm::ptr<const SceGxmTexture> textureArray)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedFragmentStateSetTexture(vm::ptr<SceGxmPrecomputedFragmentState> precomputedState, u32 textureIndex, vm::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedFragmentStateSetAllUniformBuffers(vm::ptr<SceGxmPrecomputedFragmentState> precomputedState, vm::ptr<const vm::ptr<const void>> bufferDataArray)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedFragmentStateSetUniformBuffer(vm::ptr<SceGxmPrecomputedFragmentState> precomputedState, u32 bufferIndex, vm::ptr<const void> bufferData)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedFragmentStateSetAllAuxiliarySurfaces(vm::ptr<SceGxmPrecomputedFragmentState> precomputedState, vm::ptr<const SceGxmAuxiliarySurface> auxSurfaceArray)
{
	throw __FUNCTION__;
}

u32 sceGxmGetPrecomputedDrawSize(vm::ptr<const SceGxmVertexProgram> vertexProgram)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedDrawInit(vm::ptr<SceGxmPrecomputedDraw> precomputedDraw, vm::ptr<const SceGxmVertexProgram> vertexProgram, vm::ptr<void> memBlock)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedDrawSetAllVertexStreams(vm::ptr<SceGxmPrecomputedDraw> precomputedDraw, vm::ptr<const vm::ptr<const void>> streamDataArray)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedDrawSetVertexStream(vm::ptr<SceGxmPrecomputedDraw> precomputedDraw, u32 streamIndex, vm::ptr<const void> streamData)
{
	throw __FUNCTION__;
}

void sceGxmPrecomputedDrawSetParams(vm::ptr<SceGxmPrecomputedDraw> precomputedDraw, SceGxmPrimitiveType primType, SceGxmIndexFormat indexType, vm::ptr<const void> indexData, u32 indexCount)
{
	throw __FUNCTION__;
}

void sceGxmPrecomputedDrawSetParamsInstanced(vm::ptr<SceGxmPrecomputedDraw> precomputedDraw, SceGxmPrimitiveType primType, SceGxmIndexFormat indexType, vm::ptr<const void> indexData, u32 indexCount, u32 indexWrap)
{
	throw __FUNCTION__;
}


s32 sceGxmGetRenderTargetMemSizes(vm::ptr<const SceGxmRenderTargetParams> params, vm::ptr<u32> hostMemSize, vm::ptr<u32> driverMemSize)
{
	throw __FUNCTION__;
}

s32 sceGxmCreateRenderTarget(vm::ptr<const SceGxmRenderTargetParams> params, vm::pptr<SceGxmRenderTarget> renderTarget)
{
	throw __FUNCTION__;
}

s32 sceGxmRenderTargetGetHostMem(vm::ptr<const SceGxmRenderTarget> renderTarget, vm::pptr<void> hostMem)
{
	throw __FUNCTION__;
}

s32 sceGxmRenderTargetGetDriverMemBlock(vm::ptr<const SceGxmRenderTarget> renderTarget, vm::ptr<s32> driverMemBlock)
{
	throw __FUNCTION__;
}

s32 sceGxmDestroyRenderTarget(vm::ptr<SceGxmRenderTarget> renderTarget)
{
	throw __FUNCTION__;
}

s32 sceGxmSetUniformDataF(vm::ptr<void> uniformBuffer, vm::ptr<const SceGxmProgramParameter> parameter, u32 componentOffset, u32 componentCount, vm::ptr<const float> sourceData)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceGxm, #name, name)

psv_log_base sceGxm("SceGxm", []()
{
	sceGxm.on_load = nullptr;
	sceGxm.on_unload = nullptr;
	sceGxm.on_stop = nullptr;
	sceGxm.on_error = nullptr;

	REG_FUNC(0xB0F1E4EC, sceGxmInitialize);
	REG_FUNC(0xB627DE66, sceGxmTerminate);
	//REG_FUNC(0x48C134AB, sceGxmBug9255RaisePrimitiveSplitThresholdEs2);
	//REG_FUNC(0xA3A41A42, sceGxmBug9255SetPrimitiveSplitThresholdEs2);
	REG_FUNC(0xC61E34FC, sceGxmMapMemory);
	REG_FUNC(0x828C68E8, sceGxmUnmapMemory);
	REG_FUNC(0xFA437510, sceGxmMapVertexUsseMemory);
	REG_FUNC(0x008402C6, sceGxmMapFragmentUsseMemory);
	REG_FUNC(0x099134F5, sceGxmUnmapVertexUsseMemory);
	REG_FUNC(0x80CCEDBB, sceGxmUnmapFragmentUsseMemory);
	REG_FUNC(0xEC5C26B5, sceGxmDisplayQueueAddEntry);
	REG_FUNC(0xB98C5B0D, sceGxmDisplayQueueFinish);
	REG_FUNC(0x6A6013E1, sceGxmSyncObjectCreate);
	REG_FUNC(0x889AE88C, sceGxmSyncObjectDestroy);
	REG_FUNC(0x8BDE825A, sceGxmGetNotificationRegion);
	REG_FUNC(0x9F448E79, sceGxmNotificationWait);
	REG_FUNC(0x3D25FCE9, sceGxmPadHeartbeat);
	REG_FUNC(0x47E06984, sceGxmPadTriggerGpuPaTrace);
	REG_FUNC(0x2DB6026C, sceGxmColorSurfaceGetData);
	REG_FUNC(0x200A96E1, sceGxmColorSurfaceGetDitherMode);
	REG_FUNC(0xF3C1C6C6, sceGxmColorSurfaceGetFormat);
	REG_FUNC(0x6E3FA74D, sceGxmColorSurfaceGetScaleMode);
	REG_FUNC(0xF33D9980, sceGxmColorSurfaceGetStrideInPixels);
	REG_FUNC(0x52FDE962, sceGxmColorSurfaceGetType);
	REG_FUNC(0xED0F6E25, sceGxmColorSurfaceInit);
	REG_FUNC(0x613639FA, sceGxmColorSurfaceInitDisabled);
	REG_FUNC(0x0E0EBB57, sceGxmColorSurfaceIsEnabled);
	REG_FUNC(0x07DFEE4B, sceGxmColorSurfaceGetClip);
	REG_FUNC(0x86456F7B, sceGxmColorSurfaceSetClip);
	REG_FUNC(0x537CA400, sceGxmColorSurfaceSetData);
	REG_FUNC(0x45027BAB, sceGxmColorSurfaceSetDitherMode);
	REG_FUNC(0x5F9A3A16, sceGxmColorSurfaceSetFormat);
	REG_FUNC(0x6B96EDF7, sceGxmColorSurfaceSetScaleMode);
	//REG_FUNC(0x269B56BE, sceGxmDepthStencilSurfaceGetBackgroundDepth);
	REG_FUNC(0xAAFC062B, sceGxmDepthStencilSurfaceGetBackgroundStencil);
	REG_FUNC(0x2F5CC20C, sceGxmDepthStencilSurfaceGetForceLoadMode);
	REG_FUNC(0x544AA05A, sceGxmDepthStencilSurfaceGetForceStoreMode);
	REG_FUNC(0x8504038D, sceGxmDepthStencilSurfaceGetFormat);
	REG_FUNC(0x11628789, sceGxmDepthStencilSurfaceGetStrideInSamples);
	REG_FUNC(0xCA9D41D1, sceGxmDepthStencilSurfaceInit);
	REG_FUNC(0xA41DB0D6, sceGxmDepthStencilSurfaceInitDisabled);
	REG_FUNC(0x082200E1, sceGxmDepthStencilSurfaceIsEnabled);
	//REG_FUNC(0x32F280F0, sceGxmDepthStencilSurfaceSetBackgroundDepth);
	REG_FUNC(0xF5D3F3E8, sceGxmDepthStencilSurfaceSetBackgroundStencil);
	REG_FUNC(0x0C44ACD7, sceGxmDepthStencilSurfaceSetForceLoadMode);
	REG_FUNC(0x12AAA7AF, sceGxmDepthStencilSurfaceSetForceStoreMode);
	REG_FUNC(0xF5C89643, sceGxmColorSurfaceSetGammaMode);
	REG_FUNC(0xEE0B4DF0, sceGxmColorSurfaceGetGammaMode);
	REG_FUNC(0xE0E3B3F8, sceGxmFragmentProgramGetProgram);
	REG_FUNC(0xBC52320E, sceGxmVertexProgramGetProgram);
	REG_FUNC(0xDE9D5911, sceGxmTextureGetAnisoMode);
	REG_FUNC(0x5341BD46, sceGxmTextureGetData);
	REG_FUNC(0xE868D2B3, sceGxmTextureGetFormat);
	REG_FUNC(0x5420A086, sceGxmTextureGetHeight);
	REG_FUNC(0x2DE55DA5, sceGxmTextureGetLodBias);
	REG_FUNC(0xAE7FBB51, sceGxmTextureGetMagFilter);
	REG_FUNC(0x920666C6, sceGxmTextureGetMinFilter);
	REG_FUNC(0xCE94CA15, sceGxmTextureGetMipFilter);
	REG_FUNC(0x4CC42929, sceGxmTextureGetMipmapCount);
	REG_FUNC(0xB0BD52F3, sceGxmTextureGetStride);
	REG_FUNC(0xF65D4917, sceGxmTextureGetType);
	REG_FUNC(0xC037DA83, sceGxmTextureGetUAddrMode);
	REG_FUNC(0xD2F0D9C1, sceGxmTextureGetVAddrMode);
	REG_FUNC(0x126A3EB3, sceGxmTextureGetWidth);
	REG_FUNC(0x11DC8DC9, sceGxmTextureInitCube);
	REG_FUNC(0x4811AECB, sceGxmTextureInitLinear);
	REG_FUNC(0x6679BEF0, sceGxmTextureInitLinearStrided);
	REG_FUNC(0xD572D547, sceGxmTextureInitSwizzled);
	REG_FUNC(0xE6F0DB27, sceGxmTextureInitTiled);
	REG_FUNC(0xE719CBD4, sceGxmTextureSetAnisoMode);
	REG_FUNC(0x855814C4, sceGxmTextureSetData);
	REG_FUNC(0xFC943596, sceGxmTextureSetFormat);
	REG_FUNC(0x1B20D5DF, sceGxmTextureSetHeight);
	REG_FUNC(0xB65EE6F7, sceGxmTextureSetLodBias);
	REG_FUNC(0xFA695FD7, sceGxmTextureSetMagFilter);
	REG_FUNC(0x416764E3, sceGxmTextureSetMinFilter);
	REG_FUNC(0x1CA9FE0B, sceGxmTextureSetMipFilter);
	REG_FUNC(0xD2DC4643, sceGxmTextureSetMipmapCount);
	REG_FUNC(0x58D0EB0A, sceGxmTextureSetStride);
	REG_FUNC(0x8699ECF4, sceGxmTextureSetUAddrMode);
	REG_FUNC(0xFA22F6CC, sceGxmTextureSetVAddrMode);
	REG_FUNC(0x5A690B60, sceGxmTextureSetWidth);
	REG_FUNC(0xDD6AABFA, sceGxmTextureSetPalette);
	REG_FUNC(0x0D189C30, sceGxmTextureGetPalette);
	REG_FUNC(0xA6D9F4DA, sceGxmTextureSetGammaMode);
	REG_FUNC(0xF23FCE81, sceGxmTextureGetGammaMode);
	REG_FUNC(0x85DE8506, sceGxmGetPrecomputedFragmentStateSize);
	REG_FUNC(0x9D83CA3B, sceGxmGetPrecomputedVertexStateSize);
	REG_FUNC(0x41BBD792, sceGxmGetPrecomputedDrawSize);
	REG_FUNC(0xA197F096, sceGxmPrecomputedDrawInit);
	REG_FUNC(0xB6C6F571, sceGxmPrecomputedDrawSetAllVertexStreams);
	REG_FUNC(0x884D0D08, sceGxmPrecomputedDrawSetParams);
	REG_FUNC(0x3A7B1633, sceGxmPrecomputedDrawSetParamsInstanced);
	REG_FUNC(0x6C936214, sceGxmPrecomputedDrawSetVertexStream);
	REG_FUNC(0xCECB584A, sceGxmPrecomputedFragmentStateGetDefaultUniformBuffer);
	REG_FUNC(0x91236858, sceGxmPrecomputedFragmentStateSetDefaultUniformBuffer);
	REG_FUNC(0xE297D7AF, sceGxmPrecomputedFragmentStateInit);
	REG_FUNC(0x29118BF1, sceGxmPrecomputedFragmentStateSetTexture);
	REG_FUNC(0xB452F1FB, sceGxmPrecomputedFragmentStateSetUniformBuffer);
	REG_FUNC(0xC383DE39, sceGxmPrecomputedFragmentStateSetAllTextures);
	REG_FUNC(0x5A783DC3, sceGxmPrecomputedFragmentStateSetAllUniformBuffers);
	REG_FUNC(0x9D93B63A, sceGxmPrecomputedFragmentStateSetAllAuxiliarySurfaces);
	REG_FUNC(0xBE5A68EF, sceGxmPrecomputedVertexStateGetDefaultUniformBuffer);
	REG_FUNC(0x34BF64E3, sceGxmPrecomputedVertexStateSetDefaultUniformBuffer);
	REG_FUNC(0xBE937F8D, sceGxmPrecomputedVertexStateInit);
	REG_FUNC(0x1625D348, sceGxmPrecomputedVertexStateSetTexture);
	REG_FUNC(0xDBF97ED6, sceGxmPrecomputedVertexStateSetUniformBuffer);
	REG_FUNC(0x8FF68274, sceGxmPrecomputedVertexStateSetAllTextures);
	REG_FUNC(0x0389861D, sceGxmPrecomputedVertexStateSetAllUniformBuffers);
	REG_FUNC(0x1F856E5D, sceGxmGetRenderTargetMemSizes);
	REG_FUNC(0xD56CD7B1, sceGxmCreateRenderTarget);
	REG_FUNC(0x0B94C50A, sceGxmDestroyRenderTarget);
	REG_FUNC(0xD0EDAB4C, sceGxmRenderTargetGetHostMem);
	REG_FUNC(0x49553737, sceGxmRenderTargetGetDriverMemBlock);
	REG_FUNC(0xDBA33160, sceGxmBeginScene);
	REG_FUNC(0xE84CE5B4, sceGxmCreateContext);
	REG_FUNC(0xEDDC5FB2, sceGxmDestroyContext);
	REG_FUNC(0xBC059AFC, sceGxmDraw);
	REG_FUNC(0x14C4E7D3, sceGxmDrawInstanced);
	REG_FUNC(0xED3F78B8, sceGxmDrawPrecomputed);
	REG_FUNC(0xFE300E2F, sceGxmEndScene);
	REG_FUNC(0x0733D8AE, sceGxmFinish);
	REG_FUNC(0x51FE0899, sceGxmMidSceneFlush);
	REG_FUNC(0x4FA073A6, sceGxmPopUserMarker);
	REG_FUNC(0x3276C475, sceGxmPushUserMarker);
	REG_FUNC(0x7B1FABB6, sceGxmReserveFragmentDefaultUniformBuffer);
	REG_FUNC(0x97118913, sceGxmReserveVertexDefaultUniformBuffer);
	REG_FUNC(0x91B4F7F4, sceGxmSetAuxiliarySurface);
	REG_FUNC(0x17B3BF86, sceGxmSetBackDepthBias);
	REG_FUNC(0xB042A4D2, sceGxmSetBackDepthFunc);
	REG_FUNC(0xC18B706B, sceGxmSetBackDepthWriteEnable);
	REG_FUNC(0xE26B4834, sceGxmSetBackFragmentProgramEnable);
	REG_FUNC(0xC88EB702, sceGxmSetBackLineFillLastPixelEnable);
	REG_FUNC(0x8DCB0EDB, sceGxmSetBackPointLineWidth);
	REG_FUNC(0xF66EC6FE, sceGxmSetBackPolygonMode);
	REG_FUNC(0x1A68C8D2, sceGxmSetBackStencilFunc);
	REG_FUNC(0x866A0517, sceGxmSetBackStencilRef);
	REG_FUNC(0xE1CA72AE, sceGxmSetCullMode);
	REG_FUNC(0xAD2F48D9, sceGxmSetFragmentProgram);
	REG_FUNC(0x29C34DF5, sceGxmSetFragmentTexture);
	REG_FUNC(0xEA0FC310, sceGxmSetFragmentUniformBuffer);
	REG_FUNC(0xAAA97F81, sceGxmSetFrontDepthBias);
	REG_FUNC(0x14BD831F, sceGxmSetFrontDepthFunc);
	REG_FUNC(0xF32CBF34, sceGxmSetFrontDepthWriteEnable);
	REG_FUNC(0x575958A8, sceGxmSetFrontFragmentProgramEnable);
	REG_FUNC(0x5765DE9F, sceGxmSetFrontLineFillLastPixelEnable);
	REG_FUNC(0x06752183, sceGxmSetFrontPointLineWidth);
	REG_FUNC(0xFD93209D, sceGxmSetFrontPolygonMode);
	REG_FUNC(0xB8645A9A, sceGxmSetFrontStencilFunc);
	REG_FUNC(0x8FA6FE44, sceGxmSetFrontStencilRef);
	REG_FUNC(0xF8952750, sceGxmSetPrecomputedFragmentState);
	REG_FUNC(0xB7626A93, sceGxmSetPrecomputedVertexState);
	REG_FUNC(0x70C86868, sceGxmSetRegionClip);
	REG_FUNC(0x0DE9AEB7, sceGxmSetTwoSidedEnable);
	REG_FUNC(0xC7A8CB77, sceGxmSetUserMarker);
	REG_FUNC(0x8C6A24C9, sceGxmSetValidationEnable);
	REG_FUNC(0x31FF8ABD, sceGxmSetVertexProgram);
	REG_FUNC(0x895DF2E9, sceGxmSetVertexStream);
	REG_FUNC(0x16C9D339, sceGxmSetVertexTexture);
	REG_FUNC(0xC68015E4, sceGxmSetVertexUniformBuffer);
	//REG_FUNC(0x3EB3380B, sceGxmSetViewport);
	REG_FUNC(0x814F61EB, sceGxmSetViewportEnable);
	REG_FUNC(0x7767EC49, sceGxmSetVisibilityBuffer);
	REG_FUNC(0xEED86975, sceGxmSetWBufferEnable);
	REG_FUNC(0x1BF8B853, sceGxmSetWClampEnable);
	//REG_FUNC(0xD096336E, sceGxmSetWClampValue);
	REG_FUNC(0x12625C34, sceGxmSetFrontVisibilityTestIndex);
	REG_FUNC(0xAE7886FE, sceGxmSetBackVisibilityTestIndex);
	REG_FUNC(0xD0E3CD9A, sceGxmSetFrontVisibilityTestOp);
	REG_FUNC(0xC83F0AB3, sceGxmSetBackVisibilityTestOp);
	REG_FUNC(0x30459117, sceGxmSetFrontVisibilityTestEnable);
	REG_FUNC(0x17CF46B9, sceGxmSetBackVisibilityTestEnable);
	REG_FUNC(0xED8B6C69, sceGxmProgramCheck);
	REG_FUNC(0x277794C4, sceGxmProgramFindParameterByName);
	REG_FUNC(0x7FFFDD7A, sceGxmProgramFindParameterBySemantic);
	REG_FUNC(0x06FF9151, sceGxmProgramGetParameter);
	REG_FUNC(0xD5D5FCCD, sceGxmProgramGetParameterCount);
	REG_FUNC(0xBF5E2090, sceGxmProgramGetSize);
	REG_FUNC(0x04BB3C59, sceGxmProgramGetType);
	REG_FUNC(0x89613EF2, sceGxmProgramIsDepthReplaceUsed);
	REG_FUNC(0x029B4F1C, sceGxmProgramIsDiscardUsed);
	REG_FUNC(0x8FA3F9C3, sceGxmProgramGetDefaultUniformBufferSize);
	REG_FUNC(0xDBA8D061, sceGxmProgramParameterGetArraySize);
	REG_FUNC(0x1997DC17, sceGxmProgramParameterGetCategory);
	REG_FUNC(0xBD2998D1, sceGxmProgramParameterGetComponentCount);
	REG_FUNC(0xBB58267D, sceGxmProgramParameterGetContainerIndex);
	REG_FUNC(0x6E61DDF5, sceGxmProgramParameterGetIndex);
	REG_FUNC(0x6AF88A5D, sceGxmProgramParameterGetName);
	REG_FUNC(0x5C79D59A, sceGxmProgramParameterGetResourceIndex);
	REG_FUNC(0xAAFD61D5, sceGxmProgramParameterGetSemantic);
	REG_FUNC(0xB85CC13E, sceGxmProgramParameterGetSemanticIndex);
	REG_FUNC(0x7B9023C3, sceGxmProgramParameterGetType);
	REG_FUNC(0xF7AA978B, sceGxmProgramParameterIsSamplerCube);
	REG_FUNC(0x4CD2D19F, sceGxmShaderPatcherAddRefFragmentProgram);
	REG_FUNC(0x0FD1E589, sceGxmShaderPatcherAddRefVertexProgram);
	REG_FUNC(0x05032658, sceGxmShaderPatcherCreate);
	REG_FUNC(0x4ED2E49D, sceGxmShaderPatcherCreateFragmentProgram);
	REG_FUNC(0xB7BBA6D5, sceGxmShaderPatcherCreateVertexProgram);
	REG_FUNC(0xEAA5B100, sceGxmShaderPatcherDestroy);
	REG_FUNC(0xA949A803, sceGxmShaderPatcherGetProgramFromId);
	REG_FUNC(0x2B528462, sceGxmShaderPatcherRegisterProgram);
	REG_FUNC(0xBE2743D1, sceGxmShaderPatcherReleaseFragmentProgram);
	REG_FUNC(0xAC1FF2DA, sceGxmShaderPatcherReleaseVertexProgram);
	REG_FUNC(0x8E5FCC2A, sceGxmShaderPatcherSetAuxiliarySurface);
	REG_FUNC(0xF103AF8A, sceGxmShaderPatcherUnregisterProgram);
	REG_FUNC(0x9DBBC71C, sceGxmShaderPatcherGetHostMemAllocated);
	REG_FUNC(0xC694D039, sceGxmShaderPatcherGetBufferMemAllocated);
	REG_FUNC(0x7D2F83C1, sceGxmShaderPatcherGetVertexUsseMemAllocated);
	REG_FUNC(0x3C9DDB4A, sceGxmShaderPatcherGetFragmentUsseMemAllocated);
	REG_FUNC(0x96A7E6DD, sceGxmShaderPatcherGetUserData);
	REG_FUNC(0xF9B8FCFD, sceGxmShaderPatcherSetUserData);
	REG_FUNC(0x65DD0C84, sceGxmSetUniformDataF);
});