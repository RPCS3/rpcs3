#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceGxm.h"

s32 sceGxmInitialize(vm::psv::ptr<const SceGxmInitializeParams> params)
{
	throw __FUNCTION__;
}

s32 sceGxmTerminate()
{
	throw __FUNCTION__;
}

vm::psv::ptr<volatile u32> sceGxmGetNotificationRegion()
{
	throw __FUNCTION__;
}

s32 sceGxmNotificationWait(vm::psv::ptr<const SceGxmNotification> notification)
{
	throw __FUNCTION__;
}

s32 sceGxmMapMemory(vm::psv::ptr<void> base, u32 size, SceGxmMemoryAttribFlags attr)
{
	throw __FUNCTION__;
}

s32 sceGxmUnmapMemory(vm::psv::ptr<void> base)
{
	throw __FUNCTION__;
}

s32 sceGxmMapVertexUsseMemory(vm::psv::ptr<void> base, u32 size, vm::psv::ptr<u32> offset)
{
	throw __FUNCTION__;
}

s32 sceGxmUnmapVertexUsseMemory(vm::psv::ptr<void> base)
{
	throw __FUNCTION__;
}

s32 sceGxmMapFragmentUsseMemory(vm::psv::ptr<void> base, u32 size, vm::psv::ptr<u32> offset)
{
	throw __FUNCTION__;
}

s32 sceGxmUnmapFragmentUsseMemory(vm::psv::ptr<void> base)
{
	throw __FUNCTION__;
}

s32 sceGxmDisplayQueueAddEntry(vm::psv::ptr<SceGxmSyncObject> oldBuffer, vm::psv::ptr<SceGxmSyncObject> newBuffer, vm::psv::ptr<const void> callbackData)
{
	throw __FUNCTION__;
}

s32 sceGxmDisplayQueueFinish()
{
	throw __FUNCTION__;
}

s32 sceGxmSyncObjectCreate(vm::psv::ptr<vm::psv::ptr<SceGxmSyncObject>> syncObject)
{
	throw __FUNCTION__;
}

s32 sceGxmSyncObjectDestroy(vm::psv::ptr<SceGxmSyncObject> syncObject)
{
	throw __FUNCTION__;
}


s32 sceGxmCreateContext(vm::psv::ptr<const SceGxmContextParams> params, vm::psv::ptr<vm::psv::ptr<SceGxmContext>> context)
{
	throw __FUNCTION__;
}

s32 sceGxmDestroyContext(vm::psv::ptr<SceGxmContext> context)
{
	throw __FUNCTION__;
}

void sceGxmSetValidationEnable(vm::psv::ptr<SceGxmContext> context, bool enable)
{
	throw __FUNCTION__;
}


void sceGxmSetVertexProgram(vm::psv::ptr<SceGxmContext> context, vm::psv::ptr<const SceGxmVertexProgram> vertexProgram)
{
	throw __FUNCTION__;
}

void sceGxmSetFragmentProgram(vm::psv::ptr<SceGxmContext> context, vm::psv::ptr<const SceGxmFragmentProgram> fragmentProgram)
{
	throw __FUNCTION__;
}

s32 sceGxmReserveVertexDefaultUniformBuffer(vm::psv::ptr<SceGxmContext> context, vm::psv::ptr<vm::psv::ptr<void>> uniformBuffer)
{
	throw __FUNCTION__;
}

s32 sceGxmReserveFragmentDefaultUniformBuffer(vm::psv::ptr<SceGxmContext> context, vm::psv::ptr<vm::psv::ptr<void>> uniformBuffer)
{
	throw __FUNCTION__;
}

s32 sceGxmSetVertexStream(vm::psv::ptr<SceGxmContext> context, u32 streamIndex, vm::psv::ptr<const void> streamData)
{
	throw __FUNCTION__;
}

s32 sceGxmSetVertexTexture(vm::psv::ptr<SceGxmContext> context, u32 textureIndex, vm::psv::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmSetFragmentTexture(vm::psv::ptr<SceGxmContext> context, u32 textureIndex, vm::psv::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmSetVertexUniformBuffer(vm::psv::ptr<SceGxmContext> context, u32 bufferIndex, vm::psv::ptr<const void> bufferData)
{
	throw __FUNCTION__;
}

s32 sceGxmSetFragmentUniformBuffer(vm::psv::ptr<SceGxmContext> context, u32 bufferIndex, vm::psv::ptr<const void> bufferData)
{
	throw __FUNCTION__;
}

s32 sceGxmSetAuxiliarySurface(vm::psv::ptr<SceGxmContext> context, u32 surfaceIndex, vm::psv::ptr<const SceGxmAuxiliarySurface> surface)
{
	throw __FUNCTION__;
}


void sceGxmSetPrecomputedFragmentState(vm::psv::ptr<SceGxmContext> context, vm::psv::ptr<const SceGxmPrecomputedFragmentState> precomputedState)
{
	throw __FUNCTION__;
}

void sceGxmSetPrecomputedVertexState(vm::psv::ptr<SceGxmContext> context, vm::psv::ptr<const SceGxmPrecomputedVertexState> precomputedState)
{
	throw __FUNCTION__;
}

s32 sceGxmDrawPrecomputed(vm::psv::ptr<SceGxmContext> context, vm::psv::ptr<const SceGxmPrecomputedDraw> precomputedDraw)
{
	throw __FUNCTION__;
}

s32 sceGxmDraw(vm::psv::ptr<SceGxmContext> context, SceGxmPrimitiveType primType, SceGxmIndexFormat indexType, vm::psv::ptr<const void> indexData, u32 indexCount)
{
	throw __FUNCTION__;
}

s32 sceGxmDrawInstanced(vm::psv::ptr<SceGxmContext> context, SceGxmPrimitiveType primType, SceGxmIndexFormat indexType, vm::psv::ptr<const void> indexData, u32 indexCount, u32 indexWrap)
{
	throw __FUNCTION__;
}

s32 sceGxmSetVisibilityBuffer(vm::psv::ptr<SceGxmContext> context, vm::psv::ptr<void> bufferBase, u32 stridePerCore)
{
	throw __FUNCTION__;
}

s32 sceGxmBeginScene(vm::psv::ptr<SceGxmContext> context, u32 flags, vm::psv::ptr<const SceGxmRenderTarget> renderTarget, vm::psv::ptr<const SceGxmValidRegion> validRegion, vm::psv::ptr<SceGxmSyncObject> vertexSyncObject, vm::psv::ptr<SceGxmSyncObject> fragmentSyncObject, vm::psv::ptr<const SceGxmColorSurface> colorSurface, vm::psv::ptr<const SceGxmDepthStencilSurface> depthStencil)
{
	throw __FUNCTION__;
}

s32 sceGxmMidSceneFlush(vm::psv::ptr<SceGxmContext> context, u32 flags, vm::psv::ptr<SceGxmSyncObject> vertexSyncObject, vm::psv::ptr<const SceGxmNotification> vertexNotification)
{
	throw __FUNCTION__;
}

s32 sceGxmEndScene(vm::psv::ptr<SceGxmContext> context, vm::psv::ptr<const SceGxmNotification> vertexNotification, vm::psv::ptr<const SceGxmNotification> fragmentNotification)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontDepthFunc(vm::psv::ptr<SceGxmContext> context, SceGxmDepthFunc depthFunc)
{
	throw __FUNCTION__;
}

void sceGxmSetBackDepthFunc(vm::psv::ptr<SceGxmContext> context, SceGxmDepthFunc depthFunc)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontFragmentProgramEnable(vm::psv::ptr<SceGxmContext> context, SceGxmFragmentProgramMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetBackFragmentProgramEnable(vm::psv::ptr<SceGxmContext> context, SceGxmFragmentProgramMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontDepthWriteEnable(vm::psv::ptr<SceGxmContext> context, SceGxmDepthWriteMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetBackDepthWriteEnable(vm::psv::ptr<SceGxmContext> context, SceGxmDepthWriteMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontLineFillLastPixelEnable(vm::psv::ptr<SceGxmContext> context, SceGxmLineFillLastPixelMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetBackLineFillLastPixelEnable(vm::psv::ptr<SceGxmContext> context, SceGxmLineFillLastPixelMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontStencilRef(vm::psv::ptr<SceGxmContext> context, u32 sref)
{
	throw __FUNCTION__;
}

void sceGxmSetBackStencilRef(vm::psv::ptr<SceGxmContext> context, u32 sref)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontPointLineWidth(vm::psv::ptr<SceGxmContext> context, u32 width)
{
	throw __FUNCTION__;
}

void sceGxmSetBackPointLineWidth(vm::psv::ptr<SceGxmContext> context, u32 width)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontPolygonMode(vm::psv::ptr<SceGxmContext> context, SceGxmPolygonMode mode)
{
	throw __FUNCTION__;
}

void sceGxmSetBackPolygonMode(vm::psv::ptr<SceGxmContext> context, SceGxmPolygonMode mode)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontStencilFunc(vm::psv::ptr<SceGxmContext> context, SceGxmStencilFunc func, SceGxmStencilOp stencilFail, SceGxmStencilOp depthFail, SceGxmStencilOp depthPass, u8 compareMask, u8 writeMask)
{
	throw __FUNCTION__;
}


void sceGxmSetBackStencilFunc(vm::psv::ptr<SceGxmContext> context, SceGxmStencilFunc func, SceGxmStencilOp stencilFail, SceGxmStencilOp depthFail, SceGxmStencilOp depthPass, u8 compareMask, u8 writeMask)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontDepthBias(vm::psv::ptr<SceGxmContext> context, s32 factor, s32 units)
{
	throw __FUNCTION__;
}

void sceGxmSetBackDepthBias(vm::psv::ptr<SceGxmContext> context, s32 factor, s32 units)
{
	throw __FUNCTION__;
}

void sceGxmSetTwoSidedEnable(vm::psv::ptr<SceGxmContext> context, SceGxmTwoSidedMode enable)
{
	throw __FUNCTION__;
}

//void sceGxmSetViewport(vm::psv::ptr<SceGxmContext> context, float xOffset, float xScale, float yOffset, float yScale, float zOffset, float zScale)
//{
//	throw __FUNCTION__;
//}
//
//void sceGxmSetWClampValue(vm::psv::ptr<SceGxmContext> context, float clampValue)
//{
//	throw __FUNCTION__;
//}

void sceGxmSetWClampEnable(vm::psv::ptr<SceGxmContext> context, SceGxmWClampMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetRegionClip(vm::psv::ptr<SceGxmContext> context, SceGxmRegionClipMode mode, u32 xMin, u32 yMin, u32 xMax, u32 yMax)
{
	throw __FUNCTION__;
}

void sceGxmSetCullMode(vm::psv::ptr<SceGxmContext> context, SceGxmCullMode mode)
{
	throw __FUNCTION__;
}

void sceGxmSetViewportEnable(vm::psv::ptr<SceGxmContext> context, SceGxmViewportMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetWBufferEnable(vm::psv::ptr<SceGxmContext> context, SceGxmWBufferMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontVisibilityTestIndex(vm::psv::ptr<SceGxmContext> context, u32 index)
{
	throw __FUNCTION__;
}

void sceGxmSetBackVisibilityTestIndex(vm::psv::ptr<SceGxmContext> context, u32 index)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontVisibilityTestOp(vm::psv::ptr<SceGxmContext> context, SceGxmVisibilityTestOp op)
{
	throw __FUNCTION__;
}

void sceGxmSetBackVisibilityTestOp(vm::psv::ptr<SceGxmContext> context, SceGxmVisibilityTestOp op)
{
	throw __FUNCTION__;
}

void sceGxmSetFrontVisibilityTestEnable(vm::psv::ptr<SceGxmContext> context, SceGxmVisibilityTestMode enable)
{
	throw __FUNCTION__;
}

void sceGxmSetBackVisibilityTestEnable(vm::psv::ptr<SceGxmContext> context, SceGxmVisibilityTestMode enable)
{
	throw __FUNCTION__;
}

void sceGxmFinish(vm::psv::ptr<SceGxmContext> context)
{
	throw __FUNCTION__;
}

s32 sceGxmPushUserMarker(vm::psv::ptr<SceGxmContext> context, vm::psv::ptr<const char> tag)
{
	throw __FUNCTION__;
}

s32 sceGxmPopUserMarker(vm::psv::ptr<SceGxmContext> context)
{
	throw __FUNCTION__;
}

s32 sceGxmSetUserMarker(vm::psv::ptr<SceGxmContext> context, vm::psv::ptr<const char> tag)
{
	throw __FUNCTION__;
}

s32 sceGxmPadHeartbeat(vm::psv::ptr<const SceGxmColorSurface> displaySurface, vm::psv::ptr<SceGxmSyncObject> displaySyncObject)
{
	throw __FUNCTION__;
}

s32 sceGxmPadTriggerGpuPaTrace()
{
	throw __FUNCTION__;
}

s32 sceGxmColorSurfaceInit(vm::psv::ptr<SceGxmColorSurface> surface, SceGxmColorFormat colorFormat, SceGxmColorSurfaceType surfaceType, SceGxmColorSurfaceScaleMode scaleMode, SceGxmOutputRegisterSize outputRegisterSize, u32 width, u32 height, u32 strideInPixels, vm::psv::ptr<void> data)
{
	throw __FUNCTION__;
}

s32 sceGxmColorSurfaceInitDisabled(vm::psv::ptr<SceGxmColorSurface> surface)
{
	throw __FUNCTION__;
}

bool sceGxmColorSurfaceIsEnabled(vm::psv::ptr<const SceGxmColorSurface> surface)
{
	throw __FUNCTION__;
}

void sceGxmColorSurfaceGetClip(vm::psv::ptr<const SceGxmColorSurface> surface, vm::psv::ptr<u32> xMin, vm::psv::ptr<u32> yMin, vm::psv::ptr<u32> xMax, vm::psv::ptr<u32> yMax)
{
	throw __FUNCTION__;
}

void sceGxmColorSurfaceSetClip(vm::psv::ptr<SceGxmColorSurface> surface, u32 xMin, u32 yMin, u32 xMax, u32 yMax)
{
	throw __FUNCTION__;
}

SceGxmColorSurfaceScaleMode sceGxmColorSurfaceGetScaleMode(vm::psv::ptr<const SceGxmColorSurface> surface)
{
	throw __FUNCTION__;
}

void sceGxmColorSurfaceSetScaleMode(vm::psv::ptr<SceGxmColorSurface> surface, SceGxmColorSurfaceScaleMode scaleMode)
{
	throw __FUNCTION__;
}

vm::psv::ptr<void> sceGxmColorSurfaceGetData(vm::psv::ptr<const SceGxmColorSurface> surface)
{
	throw __FUNCTION__;
}

s32 sceGxmColorSurfaceSetData(vm::psv::ptr<SceGxmColorSurface> surface, vm::psv::ptr<void> data)
{
	throw __FUNCTION__;
}

SceGxmColorFormat sceGxmColorSurfaceGetFormat(vm::psv::ptr<const SceGxmColorSurface> surface)
{
	throw __FUNCTION__;
}

s32 sceGxmColorSurfaceSetFormat(vm::psv::ptr<SceGxmColorSurface> surface, SceGxmColorFormat format)
{
	throw __FUNCTION__;
}

SceGxmColorSurfaceType sceGxmColorSurfaceGetType(vm::psv::ptr<const SceGxmColorSurface> surface)
{
	throw __FUNCTION__;
}

u32 sceGxmColorSurfaceGetStrideInPixels(vm::psv::ptr<const SceGxmColorSurface> surface)
{
	throw __FUNCTION__;
}

s32 sceGxmDepthStencilSurfaceInit(vm::psv::ptr<SceGxmDepthStencilSurface> surface, SceGxmDepthStencilFormat depthStencilFormat, SceGxmDepthStencilSurfaceType surfaceType, u32 strideInSamples, vm::psv::ptr<void> depthData, vm::psv::ptr<void> stencilData)
{
	throw __FUNCTION__;
}

s32 sceGxmDepthStencilSurfaceInitDisabled(vm::psv::ptr<SceGxmDepthStencilSurface> surface)
{
	throw __FUNCTION__;
}

//float sceGxmDepthStencilSurfaceGetBackgroundDepth(vm::psv::ptr<const SceGxmDepthStencilSurface> surface)
//{
//	throw __FUNCTION__;
//}

//void sceGxmDepthStencilSurfaceSetBackgroundDepth(vm::psv::ptr<SceGxmDepthStencilSurface> surface, float backgroundDepth)
//{
//	throw __FUNCTION__;
//}

u8 sceGxmDepthStencilSurfaceGetBackgroundStencil(vm::psv::ptr<const SceGxmDepthStencilSurface> surface)
{
	throw __FUNCTION__;
}

void sceGxmDepthStencilSurfaceSetBackgroundStencil(vm::psv::ptr<SceGxmDepthStencilSurface> surface, u8 backgroundStencil)
{
	throw __FUNCTION__;
}

bool sceGxmDepthStencilSurfaceIsEnabled(vm::psv::ptr<const SceGxmDepthStencilSurface> surface)
{
	throw __FUNCTION__;
}

void sceGxmDepthStencilSurfaceSetForceLoadMode(vm::psv::ptr<SceGxmDepthStencilSurface> surface, SceGxmDepthStencilForceLoadMode forceLoad)
{
	throw __FUNCTION__;
}

SceGxmDepthStencilForceLoadMode sceGxmDepthStencilSurfaceGetForceLoadMode(vm::psv::ptr<const SceGxmDepthStencilSurface> surface)
{
	throw __FUNCTION__;
}

void sceGxmDepthStencilSurfaceSetForceStoreMode(vm::psv::ptr<SceGxmDepthStencilSurface> surface, SceGxmDepthStencilForceStoreMode forceStore)
{
	throw __FUNCTION__;
}

SceGxmDepthStencilForceStoreMode sceGxmDepthStencilSurfaceGetForceStoreMode(vm::psv::ptr<const SceGxmDepthStencilSurface> surface)
{
	throw __FUNCTION__;
}

SceGxmColorSurfaceGammaMode sceGxmColorSurfaceGetGammaMode(vm::psv::ptr<const SceGxmColorSurface> surface)
{
	throw __FUNCTION__;
}

s32 sceGxmColorSurfaceSetGammaMode(vm::psv::ptr<SceGxmColorSurface> surface, SceGxmColorSurfaceGammaMode gammaMode)
{
	throw __FUNCTION__;
}

SceGxmColorSurfaceDitherMode sceGxmColorSurfaceGetDitherMode(vm::psv::ptr<const SceGxmColorSurface> surface)
{
	throw __FUNCTION__;
}

s32 sceGxmColorSurfaceSetDitherMode(vm::psv::ptr<SceGxmColorSurface> surface, SceGxmColorSurfaceDitherMode ditherMode)
{
	throw __FUNCTION__;
}

SceGxmDepthStencilFormat sceGxmDepthStencilSurfaceGetFormat(vm::psv::ptr<const SceGxmDepthStencilSurface> surface)
{
	throw __FUNCTION__;
}

u32 sceGxmDepthStencilSurfaceGetStrideInSamples(vm::psv::ptr<const SceGxmDepthStencilSurface> surface)
{
	throw __FUNCTION__;
}


s32 sceGxmProgramCheck(vm::psv::ptr<const SceGxmProgram> program)
{
	throw __FUNCTION__;
}

u32 sceGxmProgramGetSize(vm::psv::ptr<const SceGxmProgram> program)
{
	throw __FUNCTION__;
}

SceGxmProgramType sceGxmProgramGetType(vm::psv::ptr<const SceGxmProgram> program)
{
	throw __FUNCTION__;
}

bool sceGxmProgramIsDiscardUsed(vm::psv::ptr<const SceGxmProgram> program)
{
	throw __FUNCTION__;
}

bool sceGxmProgramIsDepthReplaceUsed(vm::psv::ptr<const SceGxmProgram> program)
{
	throw __FUNCTION__;
}

bool sceGxmProgramIsSpriteCoordUsed(vm::psv::ptr<const SceGxmProgram> program)
{
	throw __FUNCTION__;
}

u32 sceGxmProgramGetDefaultUniformBufferSize(vm::psv::ptr<const SceGxmProgram> program)
{
	throw __FUNCTION__;
}

u32 sceGxmProgramGetParameterCount(vm::psv::ptr<const SceGxmProgram> program)
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceGxmProgramParameter> sceGxmProgramGetParameter(vm::psv::ptr<const SceGxmProgram> program, u32 index)
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceGxmProgramParameter> sceGxmProgramFindParameterByName(vm::psv::ptr<const SceGxmProgram> program, vm::psv::ptr<const char> name)
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceGxmProgramParameter> sceGxmProgramFindParameterBySemantic(vm::psv::ptr<const SceGxmProgram> program, SceGxmParameterSemantic semantic, u32 index)
{
	throw __FUNCTION__;
}

u32 sceGxmProgramParameterGetIndex(vm::psv::ptr<const SceGxmProgram> program, vm::psv::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

SceGxmParameterCategory sceGxmProgramParameterGetCategory(vm::psv::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

vm::psv::ptr<const char> sceGxmProgramParameterGetName(vm::psv::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

SceGxmParameterSemantic sceGxmProgramParameterGetSemantic(vm::psv::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

u32 sceGxmProgramParameterGetSemanticIndex(vm::psv::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

SceGxmParameterType sceGxmProgramParameterGetType(vm::psv::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

u32 sceGxmProgramParameterGetComponentCount(vm::psv::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

u32 sceGxmProgramParameterGetArraySize(vm::psv::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

u32 sceGxmProgramParameterGetResourceIndex(vm::psv::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

u32 sceGxmProgramParameterGetContainerIndex(vm::psv::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

bool sceGxmProgramParameterIsSamplerCube(vm::psv::ptr<const SceGxmProgramParameter> parameter)
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceGxmProgram> sceGxmFragmentProgramGetProgram(vm::psv::ptr<const SceGxmFragmentProgram> fragmentProgram)
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceGxmProgram> sceGxmVertexProgramGetProgram(vm::psv::ptr<const SceGxmVertexProgram> vertexProgram)
{
	throw __FUNCTION__;
}


s32 sceGxmShaderPatcherCreate(vm::psv::ptr<const SceGxmShaderPatcherParams> params, vm::psv::ptr<vm::psv::ptr<SceGxmShaderPatcher>> shaderPatcher)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherSetUserData(vm::psv::ptr<SceGxmShaderPatcher> shaderPatcher, vm::psv::ptr<void> userData)
{
	throw __FUNCTION__;
}

vm::psv::ptr<void> sceGxmShaderPatcherGetUserData(vm::psv::ptr<SceGxmShaderPatcher> shaderPatcher)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherDestroy(vm::psv::ptr<SceGxmShaderPatcher> shaderPatcher)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherRegisterProgram(vm::psv::ptr<SceGxmShaderPatcher> shaderPatcher, vm::psv::ptr<const SceGxmProgram> programHeader, vm::psv::ptr<SceGxmShaderPatcherId> programId)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherUnregisterProgram(vm::psv::ptr<SceGxmShaderPatcher> shaderPatcher, SceGxmShaderPatcherId programId)
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceGxmProgram> sceGxmShaderPatcherGetProgramFromId(SceGxmShaderPatcherId programId)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherSetAuxiliarySurface(vm::psv::ptr<SceGxmShaderPatcher> shaderPatcher, u32 auxSurfaceIndex, vm::psv::ptr<const SceGxmAuxiliarySurface> auxSurface)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherCreateVertexProgram(vm::psv::ptr<SceGxmShaderPatcher> shaderPatcher, SceGxmShaderPatcherId programId, vm::psv::ptr<const SceGxmVertexAttribute> attributes, u32 attributeCount, vm::psv::ptr<const SceGxmVertexStream> streams, u32 streamCount, vm::psv::ptr<vm::psv::ptr<SceGxmVertexProgram>> vertexProgram)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherCreateFragmentProgram(vm::psv::ptr<SceGxmShaderPatcher> shaderPatcher, SceGxmShaderPatcherId programId, SceGxmOutputRegisterFormat outputFormat, SceGxmMultisampleMode multisampleMode, vm::psv::ptr<const SceGxmBlendInfo> blendInfo, vm::psv::ptr<const SceGxmProgram> vertexProgram, vm::psv::ptr<vm::psv::ptr<SceGxmFragmentProgram>> fragmentProgram)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherAddRefVertexProgram(vm::psv::ptr<SceGxmShaderPatcher> shaderPatcher, vm::psv::ptr<SceGxmVertexProgram> vertexProgram)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherAddRefFragmentProgram(vm::psv::ptr<SceGxmShaderPatcher> shaderPatcher, vm::psv::ptr<SceGxmFragmentProgram> fragmentProgram)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherReleaseVertexProgram(vm::psv::ptr<SceGxmShaderPatcher> shaderPatcher, vm::psv::ptr<SceGxmVertexProgram> vertexProgram)
{
	throw __FUNCTION__;
}

s32 sceGxmShaderPatcherReleaseFragmentProgram(vm::psv::ptr<SceGxmShaderPatcher> shaderPatcher, vm::psv::ptr<SceGxmFragmentProgram> fragmentProgram)
{
	throw __FUNCTION__;
}

u32 sceGxmShaderPatcherGetHostMemAllocated(vm::psv::ptr<const SceGxmShaderPatcher> shaderPatcher)
{
	throw __FUNCTION__;
}

u32 sceGxmShaderPatcherGetBufferMemAllocated(vm::psv::ptr<const SceGxmShaderPatcher> shaderPatcher)
{
	throw __FUNCTION__;
}

u32 sceGxmShaderPatcherGetVertexUsseMemAllocated(vm::psv::ptr<const SceGxmShaderPatcher> shaderPatcher)
{
	throw __FUNCTION__;
}

u32 sceGxmShaderPatcherGetFragmentUsseMemAllocated(vm::psv::ptr<const SceGxmShaderPatcher> shaderPatcher)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureInitSwizzled(vm::psv::ptr<SceGxmTexture> texture, vm::psv::ptr<const void> data, SceGxmTextureFormat texFormat, u32 width, u32 height, u32 mipCount)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureInitLinear(vm::psv::ptr<SceGxmTexture> texture, vm::psv::ptr<const void> data, SceGxmTextureFormat texFormat, u32 width, u32 height, u32 mipCount)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureInitLinearStrided(vm::psv::ptr<SceGxmTexture> texture, vm::psv::ptr<const void> data, SceGxmTextureFormat texFormat, u32 width, u32 height, u32 byteStride)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureInitTiled(vm::psv::ptr<SceGxmTexture> texture, vm::psv::ptr<const void> data, SceGxmTextureFormat texFormat, u32 width, u32 height, u32 mipCount)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureInitCube(vm::psv::ptr<SceGxmTexture> texture, vm::psv::ptr<const void> data, SceGxmTextureFormat texFormat, u32 width, u32 height, u32 mipCount)
{
	throw __FUNCTION__;
}

SceGxmTextureType sceGxmTextureGetType(vm::psv::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetMinFilter(vm::psv::ptr<SceGxmTexture> texture, SceGxmTextureFilter minFilter)
{
	throw __FUNCTION__;
}

SceGxmTextureFilter sceGxmTextureGetMinFilter(vm::psv::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetMagFilter(vm::psv::ptr<SceGxmTexture> texture, SceGxmTextureFilter magFilter)
{
	throw __FUNCTION__;
}

SceGxmTextureFilter sceGxmTextureGetMagFilter(vm::psv::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetMipFilter(vm::psv::ptr<SceGxmTexture> texture, SceGxmTextureMipFilter mipFilter)
{
	throw __FUNCTION__;
}

SceGxmTextureMipFilter sceGxmTextureGetMipFilter(vm::psv::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetAnisoMode(vm::psv::ptr<SceGxmTexture> texture, SceGxmTextureAnisoMode anisoMode)
{
	throw __FUNCTION__;
}

SceGxmTextureAnisoMode sceGxmTextureGetAnisoMode(vm::psv::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetUAddrMode(vm::psv::ptr<SceGxmTexture> texture, SceGxmTextureAddrMode addrMode)
{
	throw __FUNCTION__;
}

SceGxmTextureAddrMode sceGxmTextureGetUAddrMode(vm::psv::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetVAddrMode(vm::psv::ptr<SceGxmTexture> texture, SceGxmTextureAddrMode addrMode)
{
	throw __FUNCTION__;
}

SceGxmTextureAddrMode sceGxmTextureGetVAddrMode(vm::psv::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetFormat(vm::psv::ptr<SceGxmTexture> texture, SceGxmTextureFormat texFormat)
{
	throw __FUNCTION__;
}

SceGxmTextureFormat sceGxmTextureGetFormat(vm::psv::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetLodBias(vm::psv::ptr<SceGxmTexture> texture, u32 bias)
{
	throw __FUNCTION__;
}

u32 sceGxmTextureGetLodBias(vm::psv::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetStride(vm::psv::ptr<SceGxmTexture> texture, u32 byteStride)
{
	throw __FUNCTION__;
}

u32 sceGxmTextureGetStride(vm::psv::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetWidth(vm::psv::ptr<SceGxmTexture> texture, u32 width)
{
	throw __FUNCTION__;
}

u32 sceGxmTextureGetWidth(vm::psv::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetHeight(vm::psv::ptr<SceGxmTexture> texture, u32 height)
{
	throw __FUNCTION__;
}

u32 sceGxmTextureGetHeight(vm::psv::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetData(vm::psv::ptr<SceGxmTexture> texture, vm::psv::ptr<const void> data)
{
	throw __FUNCTION__;
}

vm::psv::ptr<void> sceGxmTextureGetData(vm::psv::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetMipmapCount(vm::psv::ptr<SceGxmTexture> texture, u32 mipCount)
{
	throw __FUNCTION__;
}

u32 sceGxmTextureGetMipmapCount(vm::psv::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetPalette(vm::psv::ptr<SceGxmTexture> texture, vm::psv::ptr<const void> paletteData)
{
	throw __FUNCTION__;
}

vm::psv::ptr<void> sceGxmTextureGetPalette(vm::psv::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

SceGxmTextureGammaMode sceGxmTextureGetGammaMode(vm::psv::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmTextureSetGammaMode(vm::psv::ptr<SceGxmTexture> texture, SceGxmTextureGammaMode gammaMode)
{
	throw __FUNCTION__;
}

u32 sceGxmGetPrecomputedVertexStateSize(vm::psv::ptr<const SceGxmVertexProgram> vertexProgram)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedVertexStateInit(vm::psv::ptr<SceGxmPrecomputedVertexState> precomputedState, vm::psv::ptr<const SceGxmVertexProgram> vertexProgram, vm::psv::ptr<void> memBlock)
{
	throw __FUNCTION__;
}

void sceGxmPrecomputedVertexStateSetDefaultUniformBuffer(vm::psv::ptr<SceGxmPrecomputedVertexState> precomputedState, vm::psv::ptr<void> defaultBuffer)
{
	throw __FUNCTION__;
}

vm::psv::ptr<void> sceGxmPrecomputedVertexStateGetDefaultUniformBuffer(vm::psv::ptr<const SceGxmPrecomputedVertexState> precomputedState)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedVertexStateSetAllTextures(vm::psv::ptr<SceGxmPrecomputedVertexState> precomputedState, vm::psv::ptr<const SceGxmTexture> textures)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedVertexStateSetTexture(vm::psv::ptr<SceGxmPrecomputedVertexState> precomputedState, u32 textureIndex, vm::psv::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedVertexStateSetAllUniformBuffers(vm::psv::ptr<SceGxmPrecomputedVertexState> precomputedState, vm::psv::ptr<const vm::psv::ptr<const void>> bufferDataArray)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedVertexStateSetUniformBuffer(vm::psv::ptr<SceGxmPrecomputedVertexState> precomputedState, u32 bufferIndex, vm::psv::ptr<const void> bufferData)
{
	throw __FUNCTION__;
}

u32 sceGxmGetPrecomputedFragmentStateSize(vm::psv::ptr<const SceGxmFragmentProgram> fragmentProgram)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedFragmentStateInit(vm::psv::ptr<SceGxmPrecomputedFragmentState> precomputedState, vm::psv::ptr<const SceGxmFragmentProgram> fragmentProgram, vm::psv::ptr<void> memBlock)
{
	throw __FUNCTION__;
}

void sceGxmPrecomputedFragmentStateSetDefaultUniformBuffer(vm::psv::ptr<SceGxmPrecomputedFragmentState> precomputedState, vm::psv::ptr<void> defaultBuffer)
{
	throw __FUNCTION__;
}

vm::psv::ptr<void> sceGxmPrecomputedFragmentStateGetDefaultUniformBuffer(vm::psv::ptr<const SceGxmPrecomputedFragmentState> precomputedState)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedFragmentStateSetAllTextures(vm::psv::ptr<SceGxmPrecomputedFragmentState> precomputedState, vm::psv::ptr<const SceGxmTexture> textureArray)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedFragmentStateSetTexture(vm::psv::ptr<SceGxmPrecomputedFragmentState> precomputedState, u32 textureIndex, vm::psv::ptr<const SceGxmTexture> texture)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedFragmentStateSetAllUniformBuffers(vm::psv::ptr<SceGxmPrecomputedFragmentState> precomputedState, vm::psv::ptr<const vm::psv::ptr<const void>> bufferDataArray)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedFragmentStateSetUniformBuffer(vm::psv::ptr<SceGxmPrecomputedFragmentState> precomputedState, u32 bufferIndex, vm::psv::ptr<const void> bufferData)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedFragmentStateSetAllAuxiliarySurfaces(vm::psv::ptr<SceGxmPrecomputedFragmentState> precomputedState, vm::psv::ptr<const SceGxmAuxiliarySurface> auxSurfaceArray)
{
	throw __FUNCTION__;
}

u32 sceGxmGetPrecomputedDrawSize(vm::psv::ptr<const SceGxmVertexProgram> vertexProgram)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedDrawInit(vm::psv::ptr<SceGxmPrecomputedDraw> precomputedDraw, vm::psv::ptr<const SceGxmVertexProgram> vertexProgram, vm::psv::ptr<void> memBlock)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedDrawSetAllVertexStreams(vm::psv::ptr<SceGxmPrecomputedDraw> precomputedDraw, vm::psv::ptr<const vm::psv::ptr<const void>> streamDataArray)
{
	throw __FUNCTION__;
}

s32 sceGxmPrecomputedDrawSetVertexStream(vm::psv::ptr<SceGxmPrecomputedDraw> precomputedDraw, u32 streamIndex, vm::psv::ptr<const void> streamData)
{
	throw __FUNCTION__;
}

void sceGxmPrecomputedDrawSetParams(vm::psv::ptr<SceGxmPrecomputedDraw> precomputedDraw, SceGxmPrimitiveType primType, SceGxmIndexFormat indexType, vm::psv::ptr<const void> indexData, u32 indexCount)
{
	throw __FUNCTION__;
}

void sceGxmPrecomputedDrawSetParamsInstanced(vm::psv::ptr<SceGxmPrecomputedDraw> precomputedDraw, SceGxmPrimitiveType primType, SceGxmIndexFormat indexType, vm::psv::ptr<const void> indexData, u32 indexCount, u32 indexWrap)
{
	throw __FUNCTION__;
}


s32 sceGxmGetRenderTargetMemSizes(vm::psv::ptr<const SceGxmRenderTargetParams> params, vm::psv::ptr<u32> hostMemSize, vm::psv::ptr<u32> driverMemSize)
{
	throw __FUNCTION__;
}

s32 sceGxmCreateRenderTarget(vm::psv::ptr<const SceGxmRenderTargetParams> params, vm::psv::ptr<vm::psv::ptr<SceGxmRenderTarget>> renderTarget)
{
	throw __FUNCTION__;
}

s32 sceGxmRenderTargetGetHostMem(vm::psv::ptr<const SceGxmRenderTarget> renderTarget, vm::psv::ptr<vm::psv::ptr<void>> hostMem)
{
	throw __FUNCTION__;
}

s32 sceGxmRenderTargetGetDriverMemBlock(vm::psv::ptr<const SceGxmRenderTarget> renderTarget, vm::psv::ptr<s32> driverMemBlock)
{
	throw __FUNCTION__;
}

s32 sceGxmDestroyRenderTarget(vm::psv::ptr<SceGxmRenderTarget> renderTarget)
{
	throw __FUNCTION__;
}

s32 sceGxmSetUniformDataF(vm::psv::ptr<void> uniformBuffer, vm::psv::ptr<const SceGxmProgramParameter> parameter, u32 componentOffset, u32 componentCount, vm::psv::ptr<const float> sourceData)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func<name>(nid, &sceGxm, #name, name)

psv_log_base sceGxm("SceGxm", []()
{
	sceGxm.on_load = nullptr;
	sceGxm.on_unload = nullptr;
	sceGxm.on_stop = nullptr;

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