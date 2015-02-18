#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "cellSync2.h"

extern Module cellSync2;

#ifdef PRX_DEBUG
#include "prx_libsync2.h"
u32 libsync2;
u32 libsync2_rtoc;
#endif

s64 _cellSync2MutexAttributeInitialize(vm::ptr<CellSync2MutexAttribute> attr, u32 sdkVersion)
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x16A0, libsync2_rtoc);
#else
	cellSync2.Warning("_cellSync2MutexAttributeInitialize(attr_addr=0x%x, sdkVersion=0x%x)", attr.addr(), sdkVersion);

	attr->sdkVersion = sdkVersion;
	attr->threadTypes = CELL_SYNC2_THREAD_TYPE_PPU_THREAD | CELL_SYNC2_THREAD_TYPE_PPU_FIBER |
						CELL_SYNC2_THREAD_TYPE_SPURS_TASK | CELL_SYNC2_THREAD_TYPE_SPURS_JOB |
						CELL_SYNC2_THREAD_TYPE_SPURS_JOBQUEUE_JOB;
	attr->maxWaiters = 15;
	attr->recursive = false;
	strcpy_trunc(attr->name, "CellSync2Mutex");

	return CELL_OK;
#endif
}

s64 cellSync2MutexEstimateBufferSize(vm::ptr<const CellSync2MutexAttribute> attr, vm::ptr<u32> bufferSize)
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0xC3C, libsync2_rtoc);
#else
	cellSync2.Todo("cellSync2MutexEstimateBufferSize(attr_addr=0x%x, bufferSize_addr=0x%x)", attr.addr(), bufferSize.addr());

	if (attr->maxWaiters > 32768)
		return CELL_SYNC2_ERROR_INVAL;

	return CELL_OK;
#endif
}

s64 cellSync2MutexInitialize()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x1584, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 cellSync2MutexFinalize()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x142C, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 cellSync2MutexLock()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x1734, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 cellSync2MutexTryLock()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x1A2C, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 cellSync2MutexUnlock()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x186C, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 _cellSync2CondAttributeInitialize(vm::ptr<CellSync2CondAttribute> attr, u32 sdkVersion)
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x26DC, libsync2_rtoc);
#else
	cellSync2.Warning("_cellSync2CondAttributeInitialize(attr_addr=0x%x, sdkVersion=0x%x)", attr.addr(), sdkVersion);

	attr->sdkVersion = sdkVersion;
	attr->maxWaiters = 15;
	strcpy_trunc(attr->name, "CellSync2Cond");

	return CELL_OK;
#endif
}

s64 cellSync2CondEstimateBufferSize(vm::ptr<const CellSync2CondAttribute> attr, vm::ptr<u32> bufferSize)
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x1B90, libsync2_rtoc);
#else
	cellSync2.Todo("cellSync2CondEstimateBufferSize(attr_addr=0x%x, bufferSize_addr=0x%x)", attr.addr(), bufferSize.addr());

	if (attr->maxWaiters == 0 || attr->maxWaiters > 32768)
		return CELL_SYNC2_ERROR_INVAL;

	return CELL_OK;
#endif
}

s64 cellSync2CondInitialize()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x25DC, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 cellSync2CondFinalize()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x23E0, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 cellSync2CondWait()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x283C, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 cellSync2CondSignal()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x2768, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 cellSync2CondSignalAll()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x2910, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 _cellSync2SemaphoreAttributeInitialize(vm::ptr<CellSync2SemaphoreAttribute> attr, u32 sdkVersion)
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x5644, libsync2_rtoc);
#else
	cellSync2.Warning("_cellSync2SemaphoreAttributeInitialize(attr_addr=0x%x, sdkVersion=0x%x)", attr.addr(), sdkVersion);

	attr->sdkVersion = sdkVersion;
	attr->threadTypes = CELL_SYNC2_THREAD_TYPE_PPU_THREAD | CELL_SYNC2_THREAD_TYPE_PPU_FIBER |
						CELL_SYNC2_THREAD_TYPE_SPURS_TASK | CELL_SYNC2_THREAD_TYPE_SPURS_JOB |
						CELL_SYNC2_THREAD_TYPE_SPURS_JOBQUEUE_JOB;
	attr->maxWaiters = 1;
	strcpy_trunc(attr->name, "CellSync2Semaphore");

	return CELL_OK;
#endif
}

s64 cellSync2SemaphoreEstimateBufferSize(vm::ptr<const CellSync2SemaphoreAttribute> attr, vm::ptr<u32> bufferSize)
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x4AC4, libsync2_rtoc);
#else
	cellSync2.Todo("cellSync2SemaphoreEstimateBufferSize(attr_addr=0x%x, bufferSize_addr=0x%x)", attr.addr(), bufferSize.addr());

	if (attr->maxWaiters == 0 || attr->maxWaiters > 32768)
		return CELL_SYNC2_ERROR_INVAL;

	return CELL_OK;
#endif
}

s64 cellSync2SemaphoreInitialize()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x54E0, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 cellSync2SemaphoreFinalize()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x52F0, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 cellSync2SemaphoreAcquire()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x57A4, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 cellSync2SemaphoreTryAcquire()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x56D8, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 cellSync2SemaphoreRelease()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x5870, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 cellSync2SemaphoreGetCount()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x4B4C, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 _cellSync2QueueAttributeInitialize(vm::ptr<CellSync2QueueAttribute> attr, u32 sdkVersion)
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x3C5C, libsync2_rtoc);
#else
	cellSync2.Warning("_cellSync2QueueAttributeInitialize(attr_addr=0x%x, sdkVersion=0x%x)", attr.addr(), sdkVersion);

	attr->sdkVersion = sdkVersion;
	attr->threadTypes = CELL_SYNC2_THREAD_TYPE_PPU_THREAD | CELL_SYNC2_THREAD_TYPE_PPU_FIBER |
						CELL_SYNC2_THREAD_TYPE_SPURS_TASK | CELL_SYNC2_THREAD_TYPE_SPURS_JOB |
						CELL_SYNC2_THREAD_TYPE_SPURS_JOBQUEUE_JOB;
	attr->elementSize = 16;
	attr->depth = 1024;
	attr->maxPushWaiters = 15;
	attr->maxPopWaiters = 15;
	strcpy_trunc(attr->name, "CellSync2Queue");

	return CELL_OK;
#endif
}

s64 cellSync2QueueEstimateBufferSize(vm::ptr<const CellSync2QueueAttribute> attr, vm::ptr<u32> bufferSize)
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x2A98, libsync2_rtoc);
#else
	cellSync2.Todo("cellSync2QueueEstimateBufferSize(attr_addr=0x%x, bufferSize_addr=0x%x)", attr.addr(), bufferSize.addr());

	if (attr->elementSize == 0 || attr->elementSize > 16384 || attr->elementSize % 16 || attr->depth == 0 || attr->depth > 4294967292 ||
		attr->maxPushWaiters > 32768 || attr->maxPopWaiters > 32768)
		return CELL_SYNC2_ERROR_INVAL;

	return CELL_OK;
#endif
}

s64 cellSync2QueueInitialize()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x3F98, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 cellSync2QueueFinalize()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x3C28, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 cellSync2QueuePush()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x478C, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 cellSync2QueueTryPush()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x4680, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 cellSync2QueuePop()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x4974, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 cellSync2QueueTryPop()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x4880, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 cellSync2QueueGetSize()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x2C00, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

s64 cellSync2QueueGetDepth()
{
#ifdef PRX_DEBUG
	cellSync2.Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsync2 + 0x2B90, libsync2_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
#endif
}

Module cellSync2("cellSync2", []()
{
	cellSync2.AddFunc(0x55836e73, _cellSync2MutexAttributeInitialize);
	cellSync2.AddFunc(0xd51bfae7, cellSync2MutexEstimateBufferSize);
	cellSync2.AddFunc(0xeb81a467, cellSync2MutexInitialize);
	cellSync2.AddFunc(0x27f2d61c, cellSync2MutexFinalize);
	cellSync2.AddFunc(0xa400d82e, cellSync2MutexLock);
	cellSync2.AddFunc(0xa69c749c, cellSync2MutexTryLock);
	cellSync2.AddFunc(0x0080fe88, cellSync2MutexUnlock);

	cellSync2.AddFunc(0xdf3c532a, _cellSync2CondAttributeInitialize);
	cellSync2.AddFunc(0x5b1e4d7a, cellSync2CondEstimateBufferSize);
	cellSync2.AddFunc(0x58be9a0f, cellSync2CondInitialize);
	cellSync2.AddFunc(0x63062249, cellSync2CondFinalize);
	cellSync2.AddFunc(0xbc96d751, cellSync2CondWait);
	cellSync2.AddFunc(0x871af804, cellSync2CondSignal);
	cellSync2.AddFunc(0x8aae07c2, cellSync2CondSignalAll);

	cellSync2.AddFunc(0x2d77fe17, _cellSync2SemaphoreAttributeInitialize);
	cellSync2.AddFunc(0x74c2780f, cellSync2SemaphoreEstimateBufferSize);
	cellSync2.AddFunc(0xc5dee254, cellSync2SemaphoreInitialize);
	cellSync2.AddFunc(0x164843a7, cellSync2SemaphoreFinalize);
	cellSync2.AddFunc(0xd1b0d146, cellSync2SemaphoreAcquire);
	cellSync2.AddFunc(0x5e4b0f87, cellSync2SemaphoreTryAcquire);
	cellSync2.AddFunc(0x0c2983ac, cellSync2SemaphoreRelease);
	cellSync2.AddFunc(0x4e2ee031, cellSync2SemaphoreGetCount);

	cellSync2.AddFunc(0x5e00d433, _cellSync2QueueAttributeInitialize);
	cellSync2.AddFunc(0xc08cc0f9, cellSync2QueueEstimateBufferSize);
	cellSync2.AddFunc(0xf125e044, cellSync2QueueInitialize);
	cellSync2.AddFunc(0x6af85cdf, cellSync2QueueFinalize);
	cellSync2.AddFunc(0x7d967d91, cellSync2QueuePush);
	cellSync2.AddFunc(0x7fd479fe, cellSync2QueueTryPush);
	cellSync2.AddFunc(0xd83ab0c9, cellSync2QueuePop);
	cellSync2.AddFunc(0x0c9a0ea9, cellSync2QueueTryPop);
	cellSync2.AddFunc(0x12f0a27d, cellSync2QueueGetSize);
	cellSync2.AddFunc(0xf0e1471c, cellSync2QueueGetDepth);

#ifdef PRX_DEBUG
	CallAfter([]()
	{
		if (!Memory.MainMem.GetStartAddr()) return;

		libsync2 = (u32)Memory.MainMem.AllocAlign(sizeof(libsync2_data), 0x100000);
		memcpy(vm::get_ptr<void>(libsync2), libsync2_data, sizeof(libsync2_data));
		libsync2_rtoc = libsync2 + 0xF280;

		extern Module* sysPrxForUser;
		extern Module* cellSpurs;
		extern Module* cellSpursJq;
		extern Module* cellFiber;

		FIX_IMPORT(cellSpurs, _cellSpursSendSignal                    , libsync2 + 0x61F0);
		FIX_IMPORT(cellSpursJq, cellSpursJobQueueSendSignal           , libsync2 + 0x6210);
		FIX_IMPORT(cellFiber, cellFiberPpuUtilWorkerControlSendSignal , libsync2 + 0x6230);
		FIX_IMPORT(cellFiber, cellFiberPpuSelf                        , libsync2 + 0x6250);
		FIX_IMPORT(cellFiber, cellFiberPpuWaitSignal                  , libsync2 + 0x6270);
		FIX_IMPORT(sysPrxForUser, sys_lwmutex_lock                    , libsync2 + 0x6290);
		FIX_IMPORT(sysPrxForUser, sys_lwmutex_unlock                  , libsync2 + 0x62B0);
		FIX_IMPORT(sysPrxForUser, sys_lwmutex_create                  , libsync2 + 0x62D0);
		FIX_IMPORT(sysPrxForUser, sys_ppu_thread_get_id               , libsync2 + 0x62F0);
		FIX_IMPORT(sysPrxForUser, _sys_memset                         , libsync2 + 0x6310);
		FIX_IMPORT(sysPrxForUser, _sys_printf                         , libsync2 + 0x6330);
		fix_import(sysPrxForUser, 0x9FB6228E                          , libsync2 + 0x6350);
		FIX_IMPORT(sysPrxForUser, sys_lwmutex_destroy                 , libsync2 + 0x6370);
		FIX_IMPORT(sysPrxForUser, _sys_strncpy                        , libsync2 + 0x6390);

		fix_relocs(cellSync2, libsync2, 0x73A0, 0x95A0, 0x6B90);
	});
#endif
});
