#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "cellSpurs.h"
#include "cellSpursJq.h"

Module* cellSpursJq = nullptr;

#ifdef PRX_DEBUG
#include "prx_libspurs_jq.h"
u32 libspurs_jq;
u32 libspurs_jq_rtoc;
#endif

s64 cellSpursJobQueueAttributeInitialize()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x000010, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueAttributeSetMaxGrab()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x000058, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueAttributeSetSubmitWithEntryLock()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x000098, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueAttributeSetDoBusyWaiting()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x0000BC, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueAttributeSetIsHaltOnError()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x0000E0, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueAttributeSetIsJobTypeMemoryCheck()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x000104, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueAttributeSetMaxSizeJobDescriptor()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x000128, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueAttributeSetGrabParameters()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x000178, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueSetWaitingMode()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x0001C8, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursShutdownJobQueue()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x0002F0, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursCreateJobQueueWithJobDescriptorPool()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x0003CC, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursCreateJobQueue()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x000CA8, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJoinJobQueue()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x000CF0, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePushJobListBody()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x001B24, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePushJobBody2()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x001BF0, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePushJob2Body()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x001CD0, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePushAndReleaseJobBody()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x001DC8, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePushJobBody()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x001EC8, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePushBody()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x001F90, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueueAllocateJobDescriptorBody()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x002434, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePushSync()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x002498, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePushFlush()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x002528, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueGetSpurs()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x002598, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueGetHandleCount()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x0025C4, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueGetError()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x002600, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueGetMaxSizeJobDescriptor()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x002668, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursGetJobQueueId()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x0026A4, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueGetSuspendedJobSize()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x002700, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueClose()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x002D70, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueOpen()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x002E50, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueSemaphoreTryAcquire()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x003370, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueSemaphoreAcquire()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x003378, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueSemaphoreInitialize()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x003380, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueSendSignal()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x0033E0, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueuePortGetJobQueue()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x00354C, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePortPushSync()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x003554, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePortPushFlush()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x0035C0, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePortPushJobListBody()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x003624, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePortPushJobBody()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x003A88, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePortPushJobBody2()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x003A94, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePortPushBody()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x003A98, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueuePortTrySync()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x003C38, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueuePortSync()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x003C40, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueuePortInitialize()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x003C48, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueuePortInitializeWithDescriptorBuffer()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x003D78, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueuePortFinalize()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x003E40, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePortCopyPushJobBody()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x004280, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePortCopyPushJobBody2()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x00428C, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePortCopyPushBody()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x004290, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueuePort2GetJobQueue()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x0042A4, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueuePort2PushSync()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x0042AC, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueuePort2PushFlush()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x004330, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePort2PushJobListBody()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x0043B0, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueuePort2Sync()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x0045AC, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueuePort2Create()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x0046C4, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueuePort2Destroy()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x0047E4, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueuePort2AllocateJobDescriptor()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x004928, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePort2PushAndReleaseJobBody()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x004D94, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePort2CopyPushJobBody()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x004DD0, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 _cellSpursJobQueuePort2PushJobBody()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x004E0C, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueSetExceptionEventHandler()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x004E48, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

s64 cellSpursJobQueueUnsetExceptionEventHandler()
{
#ifdef PRX_DEBUG
	cellSpursJq->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libspurs_jq + 0x004EC0, libspurs_jq_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
#endif
}

void cellSpursJq_init(Module *pxThis)
{
	cellSpursJq = pxThis;

	REG_FUNC(cellSpursJq, cellSpursJobQueueAttributeInitialize);
	REG_FUNC(cellSpursJq, cellSpursJobQueueAttributeSetMaxGrab);
	REG_FUNC(cellSpursJq, cellSpursJobQueueAttributeSetSubmitWithEntryLock);
	REG_FUNC(cellSpursJq, cellSpursJobQueueAttributeSetDoBusyWaiting);
	REG_FUNC(cellSpursJq, cellSpursJobQueueAttributeSetIsHaltOnError);
	REG_FUNC(cellSpursJq, cellSpursJobQueueAttributeSetIsJobTypeMemoryCheck);
	REG_FUNC(cellSpursJq, cellSpursJobQueueAttributeSetMaxSizeJobDescriptor);
	REG_FUNC(cellSpursJq, cellSpursJobQueueAttributeSetGrabParameters);
	REG_FUNC(cellSpursJq, cellSpursJobQueueSetWaitingMode);
	REG_FUNC(cellSpursJq, cellSpursShutdownJobQueue);
	REG_FUNC(cellSpursJq, _cellSpursCreateJobQueueWithJobDescriptorPool);
	REG_FUNC(cellSpursJq, _cellSpursCreateJobQueue);
	REG_FUNC(cellSpursJq, cellSpursJoinJobQueue);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePushJobListBody);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePushJobBody2);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePushJob2Body);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePushAndReleaseJobBody);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePushJobBody);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePushBody);
	REG_FUNC(cellSpursJq, _cellSpursJobQueueAllocateJobDescriptorBody);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePushSync);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePushFlush);
	REG_FUNC(cellSpursJq, cellSpursJobQueueGetSpurs);
	REG_FUNC(cellSpursJq, cellSpursJobQueueGetHandleCount);
	REG_FUNC(cellSpursJq, cellSpursJobQueueGetError);
	REG_FUNC(cellSpursJq, cellSpursJobQueueGetMaxSizeJobDescriptor);
	REG_FUNC(cellSpursJq, cellSpursGetJobQueueId);
	REG_FUNC(cellSpursJq, cellSpursJobQueueGetSuspendedJobSize);
	REG_FUNC(cellSpursJq, cellSpursJobQueueClose);
	REG_FUNC(cellSpursJq, cellSpursJobQueueOpen);
	REG_FUNC(cellSpursJq, cellSpursJobQueueSemaphoreTryAcquire);
	REG_FUNC(cellSpursJq, cellSpursJobQueueSemaphoreAcquire);
	REG_FUNC(cellSpursJq, cellSpursJobQueueSemaphoreInitialize);
	REG_FUNC(cellSpursJq, cellSpursJobQueueSendSignal);
	REG_FUNC(cellSpursJq, cellSpursJobQueuePortGetJobQueue);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePortPushSync);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePortPushFlush);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePortPushJobListBody);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePortPushJobBody);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePortPushJobBody2);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePortPushBody);
	REG_FUNC(cellSpursJq, cellSpursJobQueuePortTrySync);
	REG_FUNC(cellSpursJq, cellSpursJobQueuePortSync);
	REG_FUNC(cellSpursJq, cellSpursJobQueuePortInitialize);
	REG_FUNC(cellSpursJq, cellSpursJobQueuePortInitializeWithDescriptorBuffer);
	REG_FUNC(cellSpursJq, cellSpursJobQueuePortFinalize);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePortCopyPushJobBody);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePortCopyPushJobBody2);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePortCopyPushBody);
	REG_FUNC(cellSpursJq, cellSpursJobQueuePort2GetJobQueue);
	REG_FUNC(cellSpursJq, cellSpursJobQueuePort2PushSync);
	REG_FUNC(cellSpursJq, cellSpursJobQueuePort2PushFlush);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePort2PushJobListBody);
	REG_FUNC(cellSpursJq, cellSpursJobQueuePort2Sync);
	REG_FUNC(cellSpursJq, cellSpursJobQueuePort2Create);
	REG_FUNC(cellSpursJq, cellSpursJobQueuePort2Destroy);
	REG_FUNC(cellSpursJq, cellSpursJobQueuePort2AllocateJobDescriptor);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePort2PushAndReleaseJobBody);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePort2CopyPushJobBody);
	REG_FUNC(cellSpursJq, _cellSpursJobQueuePort2PushJobBody);
	REG_FUNC(cellSpursJq, cellSpursJobQueueSetExceptionEventHandler);
	REG_FUNC(cellSpursJq, cellSpursJobQueueUnsetExceptionEventHandler);

#ifdef PRX_DEBUG
	CallAfter([]()
	{
		if (!Memory.MainMem.GetStartAddr()) return;

		libspurs_jq = (u32)Memory.MainMem.AllocAlign(sizeof(libspurs_jq_data), 0x100000);
		memcpy(vm::get_ptr<void>(libspurs_jq), libspurs_jq_data, sizeof(libspurs_jq_data));
		libspurs_jq_rtoc = libspurs_jq + 0x17E80;

		extern Module* sysPrxForUser;
		extern Module* cellSpurs;
		extern Module* cellFiber;

		FIX_IMPORT(cellSpurs, cellSpursSendWorkloadSignal           , libspurs_jq + 0x6728);
		FIX_IMPORT(cellSpurs, cellSpursWorkloadAttributeSetName     , libspurs_jq + 0x6748);
		FIX_IMPORT(cellSpurs, cellSpursRemoveWorkload               , libspurs_jq + 0x6768);
		FIX_IMPORT(cellSpurs, cellSpursWaitForWorkloadShutdown      , libspurs_jq + 0x6788);
		FIX_IMPORT(cellSpurs, cellSpursWakeUp                       , libspurs_jq + 0x67A8);
		FIX_IMPORT(cellSpurs, cellSpursShutdownWorkload             , libspurs_jq + 0x67C8);
		FIX_IMPORT(cellSpurs, cellSpursAddWorkloadWithAttribute     , libspurs_jq + 0x67E8);
		FIX_IMPORT(cellSpurs, cellSpursSetExceptionEventHandler     , libspurs_jq + 0x6808);
		FIX_IMPORT(cellSpurs, _cellSpursWorkloadAttributeInitialize , libspurs_jq + 0x6828);
		FIX_IMPORT(cellFiber, cellFiberPpuSelf                      , libspurs_jq + 0x6848);
		FIX_IMPORT(cellFiber, cellFiberPpuWaitSignal                , libspurs_jq + 0x6868);
		FIX_IMPORT(sysPrxForUser, _sys_strncmp                      , libspurs_jq + 0x6888);
		FIX_IMPORT(sysPrxForUser, _sys_snprintf                     , libspurs_jq + 0x68A8);
		FIX_IMPORT(sysPrxForUser, sys_lwcond_destroy                , libspurs_jq + 0x68C8);
		FIX_IMPORT(sysPrxForUser, sys_lwmutex_create                , libspurs_jq + 0x68E8);
		FIX_IMPORT(sysPrxForUser, _sys_memset                       , libspurs_jq + 0x6908);
		FIX_IMPORT(sysPrxForUser, _sys_printf                       , libspurs_jq + 0x6928);
		fix_import(sysPrxForUser, 0x9FB6228E                        , libspurs_jq + 0x6948);
		FIX_IMPORT(sysPrxForUser, sys_lwmutex_destroy               , libspurs_jq + 0x6968);
		FIX_IMPORT(sysPrxForUser, sys_lwcond_create                 , libspurs_jq + 0x6988);
		fix_import(sysPrxForUser, 0xE75C40F2                        , libspurs_jq + 0x69A8);

		fix_relocs(cellSpursJq, libspurs_jq, 0xFF70, 0x12370, 0xED00);
	});
#endif
}