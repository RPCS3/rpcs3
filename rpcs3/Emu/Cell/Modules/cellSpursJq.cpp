#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_lwmutex.h"
#include "Emu/Cell/lv2/sys_lwcond.h"
#include "Emu/Cell/lv2/sys_spu.h"
#include "cellSpurs.h"
#include "cellSpursJq.h"

LOG_CHANNEL(cellSpursJq);

s32 cellSpursJobQueueAttributeInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueAttributeSetMaxGrab()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueAttributeSetSubmitWithEntryLock()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueAttributeSetDoBusyWaiting()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueAttributeSetIsHaltOnError()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueAttributeSetIsJobTypeMemoryCheck()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueAttributeSetMaxSizeJobDescriptor()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueAttributeSetGrabParameters()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueSetWaitingMode()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursShutdownJobQueue()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursCreateJobQueueWithJobDescriptorPool()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursCreateJobQueue()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJoinJobQueue()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePushJobListBody()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePushJobBody2()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePushJob2Body()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePushAndReleaseJobBody()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePushJobBody()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePushBody()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueueAllocateJobDescriptorBody()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePushSync()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePushFlush()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueGetSpurs()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueGetHandleCount()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueGetError()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueGetMaxSizeJobDescriptor()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursGetJobQueueId()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueGetSuspendedJobSize()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueClose()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueOpen()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueSemaphoreTryAcquire()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueSemaphoreAcquire()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueSemaphoreInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueSendSignal()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueuePortGetJobQueue()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePortPushSync()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePortPushFlush()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePortPushJobListBody()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePortPushJobBody()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePortPushJobBody2()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePortPushBody()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueuePortTrySync()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueuePortSync()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueuePortInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueuePortInitializeWithDescriptorBuffer()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueuePortFinalize()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePortCopyPushJobBody()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePortCopyPushJobBody2()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePortCopyPushBody()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueuePort2GetJobQueue()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueuePort2PushSync()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueuePort2PushFlush()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePort2PushJobListBody()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueuePort2Sync()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueuePort2Create()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueuePort2Destroy()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueuePort2AllocateJobDescriptor()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePort2PushAndReleaseJobBody()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePort2CopyPushJobBody()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 _cellSpursJobQueuePort2PushJobBody()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueSetExceptionEventHandler()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueSetExceptionEventHandler2()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

s32 cellSpursJobQueueUnsetExceptionEventHandler()
{
	UNIMPLEMENTED_FUNC(cellSpursJq);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellSpursJq)("cellSpursJq", []()
{
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
	REG_FUNC(cellSpursJq, cellSpursJobQueueSetExceptionEventHandler2);
	REG_FUNC(cellSpursJq, cellSpursJobQueueUnsetExceptionEventHandler);
});
