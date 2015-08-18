#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/IdManager.h"
#include "Emu/Event.h"

#include "Emu/Cell/SPUThread.h"
#include "Emu/SysCalls/lv2/sys_sync.h"
#include "Emu/SysCalls/lv2/sys_lwmutex.h"
#include "Emu/SysCalls/lv2/sys_lwcond.h"
#include "Emu/SysCalls/lv2/sys_spu.h"
#include "Emu/SysCalls/lv2/sys_ppu_thread.h"
#include "Emu/SysCalls/lv2/sys_memory.h"
#include "Emu/SysCalls/lv2/sys_process.h"
#include "Emu/SysCalls/lv2/sys_semaphore.h"
#include "Emu/SysCalls/lv2/sys_event.h"
#include "sysPrxForUser.h"
#include "cellSpurs.h"

//----------------------------------------------------------------------------
// Externs
//----------------------------------------------------------------------------

extern Module cellSpurs;

//----------------------------------------------------------------------------
// Function prototypes
//----------------------------------------------------------------------------

//
// SPURS SPU functions
//
bool spursKernelEntry(SPUThread & spu);

//
// SPURS utility functions
//
u32 spursGetSdkVersion();
bool spursIsLibProfLoaded();

//
// SPURS core functions
//
s32 spursCreateLv2EventQueue(PPUThread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<u32> queueId, vm::ptr<u8> port, s32 size, vm::cptr<char> name);
s32 spursAttachLv2EventQueue(PPUThread& ppu, vm::ptr<CellSpurs> spurs, u32 queue, vm::ptr<u8> port, s32 isDynamic, bool spursCreated);
s32 spursDetachLv2EventQueue(vm::ptr<CellSpurs> spurs, u8 spuPort, bool spursCreated);
void spursHandlerWaitReady(PPUThread& ppu, vm::ptr<CellSpurs> spurs);
void spursHandlerEntry(PPUThread& ppu);
s32 spursCreateHandler(vm::ptr<CellSpurs> spurs, u32 ppuPriority);
s32 spursInvokeEventHandlers(PPUThread& ppu, vm::ptr<CellSpurs::EventPortMux> eventPortMux);
s32 spursWakeUpShutdownCompletionWaiter(PPUThread& ppu, vm::ptr<CellSpurs> spurs, u32 wid);
void spursEventHelperEntry(PPUThread& ppu);
s32 spursCreateSpursEventHelper(PPUThread& ppu, vm::ptr<CellSpurs> spurs, u32 ppuPriority);
void spursInitialiseEventPortMux(vm::ptr<CellSpurs::EventPortMux> eventPortMux, u8 spuPort, u32 eventPort, u32 unknown);
s32 spursAddDefaultSystemWorkload(vm::ptr<CellSpurs> spurs, vm::cptr<u8> swlPriority, u32 swlMaxSpu, u32 swlIsPreem);
s32 spursFinalizeSpu(vm::ptr<CellSpurs> spurs);
s32 spursStopEventHelper(PPUThread& ppu, vm::ptr<CellSpurs> spurs);
s32 spursSignalToHandlerThread(PPUThread& ppu, vm::ptr<CellSpurs> spurs);
s32 spursJoinHandlerThread(PPUThread& ppu, vm::ptr<CellSpurs> spurs);
s32 spursInit(PPUThread& ppu, vm::ptr<CellSpurs> spurs, u32 revision, u32 sdkVersion, s32 nSpus, s32 spuPriority, s32 ppuPriority, u32 flags,
	vm::cptr<char> prefix, u32 prefixSize, u32 container, vm::cptr<u8> swlPriority, u32 swlMaxSpu, u32 swlIsPreem);
s32 cellSpursInitialize(PPUThread& ppu, vm::ptr<CellSpurs> spurs, s32 nSpus, s32 spuPriority, s32 ppuPriority, b8 exitIfNoWork);
s32 cellSpursInitializeWithAttribute(PPUThread& ppu, vm::ptr<CellSpurs> spurs, vm::cptr<CellSpursAttribute> attr);
s32 cellSpursInitializeWithAttribute2(PPUThread& ppu, vm::ptr<CellSpurs> spurs, vm::cptr<CellSpursAttribute> attr);
s32 _cellSpursAttributeInitialize(vm::ptr<CellSpursAttribute> attr, u32 revision, u32 sdkVersion, u32 nSpus, s32 spuPriority, s32 ppuPriority, b8 exitIfNoWork);
s32 cellSpursAttributeSetMemoryContainerForSpuThread(vm::ptr<CellSpursAttribute> attr, u32 container);
s32 cellSpursAttributeSetNamePrefix(vm::ptr<CellSpursAttribute> attr, vm::cptr<char> prefix, u32 size);
s32 cellSpursAttributeEnableSpuPrintfIfAvailable(vm::ptr<CellSpursAttribute> attr);
s32 cellSpursAttributeSetSpuThreadGroupType(vm::ptr<CellSpursAttribute> attr, s32 type);
s32 cellSpursAttributeEnableSystemWorkload(vm::ptr<CellSpursAttribute> attr, vm::cptr<u8[8]> priority, u32 maxSpu, vm::cptr<b8[8]> isPreemptible);
s32 cellSpursFinalize(vm::ptr<CellSpurs> spurs);
s32 cellSpursGetSpuThreadGroupId(vm::ptr<CellSpurs> spurs, vm::ptr<u32> group);
s32 cellSpursGetNumSpuThread(vm::ptr<CellSpurs> spurs, vm::ptr<u32> nThreads);
s32 cellSpursGetSpuThreadId(vm::ptr<CellSpurs> spurs, vm::ptr<u32> thread, vm::ptr<u32> nThreads);
s32 cellSpursSetMaxContention(vm::ptr<CellSpurs> spurs, u32 wid, u32 maxContention);
s32 cellSpursSetPriorities(vm::ptr<CellSpurs> spurs, u32 wid, vm::cptr<u8> priorities);
s32 cellSpursSetPreemptionVictimHints(vm::ptr<CellSpurs> spurs, vm::cptr<b8> isPreemptible);
s32 cellSpursAttachLv2EventQueue(PPUThread& ppu, vm::ptr<CellSpurs> spurs, u32 queue, vm::ptr<u8> port, s32 isDynamic);
s32 cellSpursDetachLv2EventQueue(vm::ptr<CellSpurs> spurs, u8 port);
s32 cellSpursEnableExceptionEventHandler(vm::ptr<CellSpurs> spurs, b8 flag);
s32 cellSpursSetGlobalExceptionEventHandler(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursGlobalExceptionEventHandler> eaHandler, vm::ptr<void> arg);
s32 cellSpursUnsetGlobalExceptionEventHandler(vm::ptr<CellSpurs> spurs);
s32 cellSpursGetInfo(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursInfo> info);

//
// SPURS SPU GUID functions
//
s32 cellSpursGetSpuGuid();

//
// SPURS trace functions
//
void spursTraceStatusUpdate(PPUThread& ppu, vm::ptr<CellSpurs> spurs);
s32 spursTraceInitialize(PPUThread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTraceInfo> buffer, u32 size, u32 mode, u32 updateStatus);
s32 cellSpursTraceInitialize(PPUThread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTraceInfo> buffer, u32 size, u32 mode);
s32 cellSpursTraceFinalize(PPUThread& ppu, vm::ptr<CellSpurs> spurs);
s32 spursTraceStart(PPUThread& ppu, vm::ptr<CellSpurs> spurs, u32 updateStatus);
s32 cellSpursTraceStart(PPUThread& ppu, vm::ptr<CellSpurs> spurs);
s32 spursTraceStop(PPUThread& ppu, vm::ptr<CellSpurs> spurs, u32 updateStatus);
s32 cellSpursTraceStop(PPUThread& ppu, vm::ptr<CellSpurs> spurs);

//
// SPURS policy module functions
//
s32 _cellSpursWorkloadAttributeInitialize(vm::ptr<CellSpursWorkloadAttribute> attr, u32 revision, u32 sdkVersion, vm::cptr<void> pm, u32 size, u64 data, vm::cptr<u8[8]> priority, u32 minCnt, u32 maxCnt);
s32 cellSpursWorkloadAttributeSetName(vm::ptr<CellSpursWorkloadAttribute> attr, vm::cptr<char> nameClass, vm::cptr<char> nameInstance);
s32 cellSpursWorkloadAttributeSetShutdownCompletionEventHook(vm::ptr<CellSpursWorkloadAttribute> attr, vm::ptr<CellSpursShutdownCompletionEventHook> hook, vm::ptr<void> arg);
s32 spursAddWorkload(vm::ptr<CellSpurs> spurs, vm::ptr<u32> wid, vm::cptr<void> pm, u32 size, u64 data, const u8 priorityTable[], u32 minContention, u32 maxContention,
	vm::cptr<char> nameClass, vm::cptr<char> nameInstance, vm::ptr<CellSpursShutdownCompletionEventHook> hook, vm::ptr<void> hookArg);
s32 cellSpursAddWorkload(vm::ptr<CellSpurs> spurs, vm::ptr<u32> wid, vm::cptr<void> pm, u32 size, u64 data, vm::cptr<u8[8]> priority, u32 minCnt, u32 maxCnt);
s32 cellSpursAddWorkloadWithAttribute(vm::ptr<CellSpurs> spurs, vm::ptr<u32> wid, vm::cptr<CellSpursWorkloadAttribute> attr);
s32 cellSpursShutdownWorkload();
s32 cellSpursWaitForWorkloadShutdown();
s32 cellSpursRemoveWorkload();
s32 cellSpursWakeUp(PPUThread& ppu, vm::ptr<CellSpurs> spurs);
s32 cellSpursSendWorkloadSignal(vm::ptr<CellSpurs> spurs, u32 wid);
s32 cellSpursGetWorkloadFlag(vm::ptr<CellSpurs> spurs, vm::pptr<CellSpursWorkloadFlag> flag);
s32 cellSpursReadyCountStore(vm::ptr<CellSpurs> spurs, u32 wid, u32 value);
s32 cellSpursReadyCountSwap();
s32 cellSpursReadyCountCompareAndSwap();
s32 cellSpursReadyCountAdd();
s32 cellSpursGetWorkloadData(vm::ptr<CellSpurs> spurs, vm::ptr<u64> data, u32 wid);
s32 cellSpursGetWorkloadInfo();
s32 cellSpursSetExceptionEventHandler();
s32 cellSpursUnsetExceptionEventHandler();
s32 _cellSpursWorkloadFlagReceiver(vm::ptr<CellSpurs> spurs, u32 wid, u32 is_set);
s32 _cellSpursWorkloadFlagReceiver2();
s32 cellSpursRequestIdleSpu();

//
// SPURS taskset functions
//
s32 spursCreateTaskset(PPUThread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, u64 args, vm::cptr<u8[8]> priority, u32 max_contention, vm::cptr<char> name, u32 size, s32 enable_clear_ls);
s32 cellSpursCreateTasksetWithAttribute(PPUThread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursTasksetAttribute> attr);
s32 cellSpursCreateTaskset(PPUThread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, u64 args, vm::cptr<u8[8]> priority, u32 maxContention);
s32 cellSpursJoinTaskset(vm::ptr<CellSpursTaskset> taskset);
s32 cellSpursGetTasksetId(vm::ptr<CellSpursTaskset> taskset, vm::ptr<u32> wid);
s32 cellSpursShutdownTaskset(vm::ptr<CellSpursTaskset> taskset);
s32 cellSpursTasksetAttributeSetName(vm::ptr<CellSpursTasksetAttribute> attr, vm::cptr<char> name);
s32 cellSpursTasksetAttributeSetTasksetSize(vm::ptr<CellSpursTasksetAttribute> attr, u32 size);
s32 cellSpursTasksetAttributeEnableClearLS(vm::ptr<CellSpursTasksetAttribute> attr, s32 enable);
s32 _cellSpursTasksetAttribute2Initialize(vm::ptr<CellSpursTasksetAttribute2> attribute, u32 revision);
s32 cellSpursCreateTaskset2(PPUThread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursTasksetAttribute2> attr);
s32 cellSpursDestroyTaskset2();
s32 cellSpursTasksetSetExceptionEventHandler(vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursTasksetExceptionEventHandler> handler, vm::ptr<u64> arg);
s32 cellSpursTasksetUnsetExceptionEventHandler(vm::ptr<CellSpursTaskset> taskset);
s32 cellSpursLookUpTasksetAddress(PPUThread& ppu, vm::ptr<CellSpurs> spurs, vm::pptr<CellSpursTaskset> taskset, u32 id);
s32 cellSpursTasksetGetSpursAddress(vm::cptr<CellSpursTaskset> taskset, vm::ptr<u32> spurs);
s32 cellSpursGetTasksetInfo();
s32 _cellSpursTasksetAttributeInitialize(vm::ptr<CellSpursTasksetAttribute> attribute, u32 revision, u32 sdk_version, u64 args, vm::cptr<u8> priority, u32 max_contention);

//
// SPURS task functions
//
s32 spursCreateTask(vm::ptr<CellSpursTaskset> taskset, vm::ptr<u32> task_id, vm::cptr<void> elf, vm::cptr<void> context, u32 size, vm::ptr<CellSpursTaskLsPattern> ls_pattern, vm::ptr<CellSpursTaskArgument> arg);
s32 spursTaskStart(PPUThread& ppu, vm::ptr<CellSpursTaskset> taskset, u32 taskId);
s32 cellSpursCreateTask(PPUThread& ppu, vm::ptr<CellSpursTaskset> taskset, vm::ptr<u32> taskId, vm::cptr<void> elf, vm::cptr<void> context, u32 size, vm::ptr<CellSpursTaskLsPattern> lsPattern, vm::ptr<CellSpursTaskArgument> argument);
s32 _cellSpursSendSignal(PPUThread& ppu, vm::ptr<CellSpursTaskset> taskset, u32 taskId);
s32 cellSpursCreateTaskWithAttribute();
s32 cellSpursTaskExitCodeGet();
s32 cellSpursTaskExitCodeInitialize();
s32 cellSpursTaskExitCodeTryGet();
s32 cellSpursTaskGetLoadableSegmentPattern();
s32 cellSpursTaskGetReadOnlyAreaPattern();
s32 cellSpursTaskGenerateLsPattern();
s32 _cellSpursTaskAttributeInitialize();
s32 cellSpursTaskAttributeSetExitCodeContainer();
s32 _cellSpursTaskAttribute2Initialize(vm::ptr<CellSpursTaskAttribute2> attribute, u32 revision);
s32 cellSpursTaskGetContextSaveAreaSize();
s32 cellSpursCreateTask2();
s32 cellSpursJoinTask2();
s32 cellSpursTryJoinTask2();
s32 cellSpursCreateTask2WithBinInfo();

//
// SPURS event flag functions
//
s32 _cellSpursEventFlagInitialize(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursEventFlag> eventFlag, u32 flagClearMode, u32 flagDirection);
s32 cellSpursEventFlagClear(vm::ptr<CellSpursEventFlag> eventFlag, u16 bits);
s32 cellSpursEventFlagSet(PPUThread& ppu, vm::ptr<CellSpursEventFlag> eventFlag, u16 bits);
s32 spursEventFlagWait(PPUThread& ppu, vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u16> mask, u32 mode, u32 block);
s32 cellSpursEventFlagWait(PPUThread& ppu, vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u16> mask, u32 mode);
s32 cellSpursEventFlagTryWait(PPUThread& ppu, vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u16> mask, u32 mode);
s32 cellSpursEventFlagAttachLv2EventQueue(PPUThread& ppu, vm::ptr<CellSpursEventFlag> eventFlag);
s32 cellSpursEventFlagDetachLv2EventQueue(vm::ptr<CellSpursEventFlag> eventFlag);
s32 cellSpursEventFlagGetDirection(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u32> direction);
s32 cellSpursEventFlagGetClearMode(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u32> clear_mode);
s32 cellSpursEventFlagGetTasksetAddress(vm::ptr<CellSpursEventFlag> eventFlag, vm::pptr<CellSpursTaskset> taskset);

//
// SPURS lock free queue functions
//
s32 _cellSpursLFQueueInitialize(vm::ptr<void> pTasksetOrSpurs, vm::ptr<CellSpursLFQueue> pQueue, vm::cptr<void> buffer, u32 size, u32 depth, u32 direction);
s32 _cellSpursLFQueuePushBody();
s32 cellSpursLFQueueAttachLv2EventQueue(vm::ptr<CellSyncLFQueue> queue);
s32 cellSpursLFQueueDetachLv2EventQueue(vm::ptr<CellSyncLFQueue> queue);
s32 _cellSpursLFQueuePopBody();
s32 cellSpursLFQueueGetTasksetAddress();

//
// SPURS queue functions
//
s32 _cellSpursQueueInitialize();
s32 cellSpursQueuePopBody();
s32 cellSpursQueuePushBody();
s32 cellSpursQueueAttachLv2EventQueue();
s32 cellSpursQueueDetachLv2EventQueue();
s32 cellSpursQueueGetTasksetAddress();
s32 cellSpursQueueClear();
s32 cellSpursQueueDepth();
s32 cellSpursQueueGetEntrySize();
s32 cellSpursQueueSize();
s32 cellSpursQueueGetDirection();

//
// SPURS barrier functions
//
s32 cellSpursBarrierInitialize();
s32 cellSpursBarrierGetTasksetAddress();

//
// SPURS semaphore functions
//
s32 _cellSpursSemaphoreInitialize();
s32 cellSpursSemaphoreGetTasksetAddress();

//
// SPURS job chain functions
//
s32 cellSpursCreateJobChainWithAttribute();
s32 cellSpursCreateJobChain();
s32 cellSpursJoinJobChain();
s32 cellSpursKickJobChain();
s32 _cellSpursJobChainAttributeInitialize();
s32 cellSpursGetJobChainId();
s32 cellSpursJobChainSetExceptionEventHandler();
s32 cellSpursJobChainUnsetExceptionEventHandler();
s32 cellSpursGetJobChainInfo();
s32 cellSpursJobChainGetSpursAddress();
s32 cellSpursJobGuardInitialize();
s32 cellSpursJobChainAttributeSetName();
s32 cellSpursShutdownJobChain();
s32 cellSpursJobChainAttributeSetHaltOnError();
s32 cellSpursJobChainAttributeSetJobTypeMemoryCheck();
s32 cellSpursJobGuardNotify();
s32 cellSpursJobGuardReset();
s32 cellSpursRunJobChain();
s32 cellSpursJobChainGetError();
s32 cellSpursGetJobPipelineInfo();
s32 cellSpursJobSetMaxGrab();
s32 cellSpursJobHeaderSetJobbin2Param();
s32 cellSpursAddUrgentCommand();
s32 cellSpursAddUrgentCall();

//----------------------------------------------------------------------------
// SPURS utility functions
//----------------------------------------------------------------------------

/// Get the version of SDK used by this process
u32 spursGetSdkVersion()
{
	s32 version;

	if (s32 rc = process_get_sdk_version(process_getpid(), version))
	{
		throw EXCEPTION("process_get_sdk_version() failed (0x%x)", rc);
	}

	return version == -1 ? 0x465000 : version;
}

/// Check whether libprof is loaded
bool spursIsLibProfLoaded()
{
	return false;
}

//----------------------------------------------------------------------------
// SPURS core functions
//----------------------------------------------------------------------------

/// Create an LV2 event queue and attach it to the SPURS instance
s32 spursCreateLv2EventQueue(PPUThread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<u32> queueId, vm::ptr<u8> port, s32 size, vm::cptr<char> name)
{
	const vm::var<sys_event_queue_attribute_t> attr(ppu);

	auto sys_event_queue_attribute_initialize = [](vm::ptr<sys_event_queue_attribute_t> attr)
	{
		attr->protocol = SYS_SYNC_PRIORITY;
		attr->type     = SYS_PPU_QUEUE;
		attr->name[0]  = '\0';
	};

	sys_event_queue_attribute_initialize(attr);
	std::memcpy(attr->name, name.get_ptr(), sizeof(attr->name));

	if (s32 rc = sys_event_queue_create(queueId, attr, SYS_EVENT_QUEUE_LOCAL, size))
	{
		return rc;
	}

	if (s32 rc = spursAttachLv2EventQueue(ppu, spurs, *queueId, port, 1 /*isDynamic*/, true /*spursCreated*/))
	{
		sys_event_queue_destroy(*queueId, SYS_EVENT_QUEUE_DESTROY_FORCE);
	}

	return CELL_OK;
}

/// Attach an LV2 event queue to the SPURS instance
s32 spursAttachLv2EventQueue(PPUThread& ppu, vm::ptr<CellSpurs> spurs, u32 queue, vm::ptr<u8> port, s32 isDynamic, bool spursCreated)
{
	if (!spurs || !port)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (spurs->exception.data())
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	s32 sdkVer   = spursGetSdkVersion();
	u8 _port     = 0x3f;
	u64 portMask = 0;

	if (isDynamic == 0)
	{
		_port = *port;
		if (_port > 0x3f)
		{
			return CELL_SPURS_CORE_ERROR_INVAL;
		}

		if (sdkVer >= 0x180000 && _port > 0xf)
		{
			return CELL_SPURS_CORE_ERROR_PERM;
		}
	}

	for (u32 i = isDynamic ? 0x10 : _port; i <= _port; i++)
	{
		portMask |= 1ull << (i);
	}

	if (s32 res = sys_spu_thread_group_connect_event_all_threads(spurs->spuTG, queue, portMask, port))
	{
		if (res == CELL_EISCONN)
		{
			return CELL_SPURS_CORE_ERROR_BUSY;
		}

		return res;
	}

	if (!spursCreated)
	{
		spurs->spuPortBits |= 1ull << *port;
	}

	return CELL_OK;
}

/// Detach an LV2 event queue from the SPURS instance
s32 spursDetachLv2EventQueue(vm::ptr<CellSpurs> spurs, u8 spuPort, bool spursCreated)
{
	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spursCreated && spurs->exception.data())
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	if (spuPort > 0x3F)
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	auto sdkVer = spursGetSdkVersion();
	if (!spursCreated)
	{
		auto mask = 1ull << spuPort;
		if (sdkVer >= 0x180000)
		{
			if ((spurs->spuPortBits.load() & mask) == 0)
			{
				return CELL_SPURS_CORE_ERROR_SRCH;
			}
		}

		spurs->spuPortBits &= ~mask;
	}

	return CELL_OK;
}

/// Wait until a workload in the SPURS instance becomes ready
void spursHandlerWaitReady(PPUThread& ppu, vm::ptr<CellSpurs> spurs)
{
	if (s32 rc = sys_lwmutex_lock(ppu, spurs.ptr(&CellSpurs::mutex), 0))
	{
		throw EXCEPTION("sys_lwmutex_lock() failed (0x%x)", rc);
	}

	while (true)
	{
		CHECK_EMU_STATUS;

		if (spurs->handlerExiting.load())
		{
			extern u32 g_ppu_func_index__sys_lwmutex_unlock; // test

			if (s32 rc = CALL_FUNC(ppu, sys_lwmutex_unlock, ppu, spurs.ptr(&CellSpurs::mutex)))
			{
				throw EXCEPTION("sys_lwmutex_unlock() failed (0x%x)", rc);
			}

			sys_ppu_thread_exit(ppu, 0);
		}

		// Find a runnable workload
		spurs->handlerDirty.store(0);
		if (spurs->exception == 0)
		{
			bool foundRunnableWorkload = false;
			for (u32 i = 0; i < 16; i++)
			{
				if (spurs->wklState1[i].load() == SPURS_WKL_STATE_RUNNABLE &&
					*((u64*)spurs->wklInfo1[i].priority) != 0 &&
					spurs->wklMaxContention[i].load() & 0x0F)
				{
					if (spurs->wklReadyCount1[i].load() ||
						spurs->wklSignal1.load() & (0x8000u >> i) ||
						(spurs->wklFlag.flag.load() == 0 &&
							spurs->wklFlagReceiver.load() == (u8)i))
					{
						foundRunnableWorkload = true;
						break;
					}
				}
			}

			if (spurs->flags1 & SF1_32_WORKLOADS)
			{
				for (u32 i = 0; i < 16; i++)
				{
					if (spurs->wklState2[i].load() == SPURS_WKL_STATE_RUNNABLE &&
						*((u64*)spurs->wklInfo2[i].priority) != 0 &&
						spurs->wklMaxContention[i].load() & 0xF0)
					{
						if (spurs->wklIdleSpuCountOrReadyCount2[i].load() ||
							spurs->wklSignal2.load() & (0x8000u >> i) ||
							(spurs->wklFlag.flag.load() == 0 &&
								spurs->wklFlagReceiver.load() == (u8)i + 0x10))
						{
							foundRunnableWorkload = true;
							break;
						}
					}
				}
			}

			if (foundRunnableWorkload) {
				break;
			}
		}

		// If we reach it means there are no runnable workloads in this SPURS instance.
		// Wait until some workload becomes ready.
		spurs->handlerWaiting.store(1);
		if (spurs->handlerDirty.load() == 0)
		{
			if (s32 rc = sys_lwcond_wait(ppu, spurs.ptr(&CellSpurs::cond), 0))
			{
				throw EXCEPTION("sys_lwcond_wait() failed (0x%x)", rc);
			}
		}

		spurs->handlerWaiting.store(0);
	}

	// If we reach here then a runnable workload was found
	if (s32 rc = sys_lwmutex_unlock(ppu, spurs.ptr(&CellSpurs::mutex)))
	{
		throw EXCEPTION("sys_lwmutex_unlock() failed (0x%x)", rc);
	}
}

/// Entry point of the SPURS handler thread. This thread is responsible for starting the SPURS SPU thread group.
void spursHandlerEntry(PPUThread& ppu)
{
	auto spurs = vm::ptr<CellSpurs>::make(VM_CAST(ppu.GPR[3]));

	if (spurs->flags & SAF_UNKNOWN_FLAG_30)
	{
		sys_ppu_thread_exit(ppu, 0);
	}

	while (true)
	{
		CHECK_EMU_STATUS;

		if (spurs->flags1 & SF1_EXIT_IF_NO_WORK)
		{
			spursHandlerWaitReady(ppu, spurs);
		}

		if (s32 rc = sys_spu_thread_group_start(spurs->spuTG))
		{
			throw EXCEPTION("sys_spu_thread_group_start() failed (0x%x)", rc);
		}

		if (s32 rc = sys_spu_thread_group_join(spurs->spuTG, vm::null, vm::null))
		{
			if (rc == CELL_ESTAT)
			{
				sys_ppu_thread_exit(ppu, 0);
			}

			throw EXCEPTION("sys_spu_thread_group_join() failed (0x%x)", rc);
		}

		if ((spurs->flags1 & SF1_EXIT_IF_NO_WORK) == 0)
		{
			if (spurs->handlerExiting.load() != 1)
			{
				throw EXCEPTION("Unexpected handlerExiting value (false)");
			}

			sys_ppu_thread_exit(ppu, 0);
		}
	}
}

/// Create the SPURS handler thread
s32 spursCreateHandler(vm::ptr<CellSpurs> spurs, u32 ppuPriority)
{
	// create joinable thread with stackSize = 0x4000 and custom task
	spurs->ppu0 = ppu_thread_create(0, spurs.addr(), ppuPriority, 0x4000, true, false, std::string(spurs->prefix, spurs->prefixSize) + "SpursHdlr0", spursHandlerEntry);
	return CELL_OK;
}

/// Invoke event handlers
s32 spursInvokeEventHandlers(PPUThread& ppu, vm::ptr<CellSpurs::EventPortMux> eventPortMux)
{
	if (eventPortMux->reqPending.exchange(0).data())
	{
		const vm::ptr<CellSpurs::EventHandlerListNode> handlerList = eventPortMux->handlerList.exchange(vm::null);

		for (auto node = handlerList; node; node = node->next)
		{
			node->handler(ppu, eventPortMux, node->data);
		}
	}

	return CELL_OK;
}

// Invoke workload shutdown completion callbacks
s32 spursWakeUpShutdownCompletionWaiter(PPUThread& ppu, vm::ptr<CellSpurs> spurs, u32 wid)
{
	if (!spurs)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (wid >= (u32)(spurs->flags1 & SF1_32_WORKLOADS ? CELL_SPURS_MAX_WORKLOAD2 : CELL_SPURS_MAX_WORKLOAD))
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}

	if ((spurs->wklEnabled.load() & (0x80000000u >> wid)) == 0)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_SRCH;
	}

	const u8 wklState = wid < CELL_SPURS_MAX_WORKLOAD ? spurs->wklState1[wid].load() : spurs->wklState2[wid & 0x0F].load();

	if (wklState != SPURS_WKL_STATE_REMOVABLE)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	auto& wklF     = wid < CELL_SPURS_MAX_WORKLOAD ? spurs->wklF1[wid] : spurs->wklF2[wid & 0x0F];
	auto& wklEvent = wid < CELL_SPURS_MAX_WORKLOAD ? spurs->wklEvent1[wid] : spurs->wklEvent2[wid & 0x0F];

	if (wklF.hook)
	{
		wklF.hook(ppu, spurs, wid, wklF.hookArg);

		assert(wklEvent.load() & 0x01);
		assert(wklEvent.load() & 0x02);
		assert((wklEvent.load() & 0x20) == 0);
		wklEvent |= 0x20;
	}

	s32 rc = CELL_OK;
	if (!wklF.hook || wklEvent.load() & 0x10)
	{
		assert(wklF.x28 == 2);
		rc = sys_semaphore_post((u32)wklF.sem, 1);
	}

	return rc;
}

/// Entry point of the SPURS event helper thread
void spursEventHelperEntry(PPUThread& ppu)
{
	const auto spurs = vm::ptr<CellSpurs>::make(VM_CAST(ppu.GPR[3]));

	bool terminate = false;

	const vm::var<sys_event_t> events(ppu, 8);
	const vm::var<u32>         count(ppu);

	while (!terminate)
	{
		if (s32 rc = sys_event_queue_receive(ppu, spurs->eventQueue, vm::null, 0 /*timeout*/))
		{
			throw EXCEPTION("sys_event_queue_receive() failed (0x%x)", rc);
		}

		const u64 event_src   = ppu.GPR[4];
		const u64 event_data1 = ppu.GPR[5];
		const u64 event_data2 = ppu.GPR[6];
		const u64 event_data3 = ppu.GPR[7];

		if (event_src == SYS_SPU_THREAD_EVENT_EXCEPTION_KEY)
		{
			spurs->exception = 1;

			events[0].source = event_src;
			events[0].data1 = event_data1;
			events[0].data2 = event_data2;
			events[0].data3 = event_data3;

			if (sys_event_queue_tryreceive(spurs->eventQueue, events + 1, 7, count) != CELL_OK)
			{
				continue;
			}

			// TODO: Examine LS and dump exception details

			for (auto i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++)
			{
				sys_semaphore_post((u32)spurs->wklF1[i].sem, 1);
				if (spurs->flags1 & SF1_32_WORKLOADS)
				{
					sys_semaphore_post((u32)spurs->wklF2[i].sem, 1);
				}
			}
		}
		else
		{
			const u32 data0 = event_data2 & 0x00FFFFFF;

			if (data0 == 1)
			{
				terminate = true;
			}
			else if (data0 < 1)
			{
				const u32 shutdownMask = (u32)event_data3;

				for (auto wid = 0; wid < CELL_SPURS_MAX_WORKLOAD; wid++)
				{
					if (shutdownMask & (0x80000000u >> wid))
					{
						if (s32 rc = spursWakeUpShutdownCompletionWaiter(ppu, spurs, wid))
						{
							throw EXCEPTION("spursWakeUpShutdownCompletionWaiter() failed (0x%x)", rc);
						}
					}

					if ((spurs->flags1 & SF1_32_WORKLOADS) && (shutdownMask & (0x8000 >> wid)))
					{
						if (s32 rc = spursWakeUpShutdownCompletionWaiter(ppu, spurs, wid + 0x10))
						{
							throw EXCEPTION("spursWakeUpShutdownCompletionWaiter() failed (0x%x)", rc);
						}
					}
				}
			}
			else if (data0 == 2)
			{
				if (s32 rc = sys_semaphore_post((u32)spurs->semPrv, 1))
				{
					throw EXCEPTION("sys_semaphore_post() failed (0x%x)", rc);
				}
			}
			else if (data0 == 3)
			{
				if (s32 rc = spursInvokeEventHandlers(ppu, spurs.ptr(&CellSpurs::eventPortMux)))
				{
					throw EXCEPTION("spursInvokeEventHandlers() failed (0x%x)", rc);
				}
			}
			else
			{
				throw EXCEPTION("Unexpected value (data0=0x%x)", data0);
			}
		}
	}
}

/// Create the SPURS event helper thread
s32 spursCreateSpursEventHelper(PPUThread& ppu, vm::ptr<CellSpurs> spurs, u32 ppuPriority)
{
	const vm::var<char> evqName(ppu, 8);
	memcpy(evqName.get_ptr(), "_spuPrv", 8);

	if (s32 rc = spursCreateLv2EventQueue(ppu, spurs, spurs.ptr(&CellSpurs::eventQueue), spurs.ptr(&CellSpurs::spuPort), 0x2A /*size*/, evqName))
	{
		return rc;
	}

	if (s32 rc = sys_event_port_create(spurs.ptr(&CellSpurs::eventPort), SYS_EVENT_PORT_LOCAL, SYS_EVENT_PORT_NO_NAME))
	{
		if (s32 rc2 = spursDetachLv2EventQueue(spurs, spurs->spuPort, true /*spursCreated*/))
		{
			return CELL_SPURS_CORE_ERROR_AGAIN;
		}

		sys_event_queue_destroy(spurs->eventQueue, SYS_EVENT_QUEUE_DESTROY_FORCE);
		return CELL_SPURS_CORE_ERROR_AGAIN;
	}

	if (s32 rc = sys_event_port_connect_local(spurs->eventPort, spurs->eventQueue))
	{
		sys_event_port_destroy(spurs->eventPort);

		if (s32 rc2 = spursDetachLv2EventQueue(spurs, spurs->spuPort, true /*spursCreated*/))
		{
			return CELL_SPURS_CORE_ERROR_STAT;
		}

		sys_event_queue_destroy(spurs->eventQueue, SYS_EVENT_QUEUE_DESTROY_FORCE);
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	// create joinable thread with stackSize = 0x8000 and custom task
	const u32 tid = ppu_thread_create(0, spurs.addr(), ppuPriority, 0x8000, true, false, std::string(spurs->prefix, spurs->prefixSize) + "SpursHdlr1", spursEventHelperEntry);

	// cannot happen in current implementation
	if (tid == 0)
	{
		sys_event_port_disconnect(spurs->eventPort);
		sys_event_port_destroy(spurs->eventPort);

		if (s32 rc = spursDetachLv2EventQueue(spurs, spurs->spuPort, true /*spursCreated*/))
		{
			return CELL_SPURS_CORE_ERROR_STAT;
		}

		sys_event_queue_destroy(spurs->eventQueue, SYS_EVENT_QUEUE_DESTROY_FORCE);
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	spurs->ppu1 = tid;
	return CELL_OK;
}

/// Initialise the event port multiplexor structure
void spursInitialiseEventPortMux(vm::ptr<CellSpurs::EventPortMux> eventPortMux, u8 spuPort, u32 eventPort, u32 unknown)
{
	memset(eventPortMux.get_ptr(), 0, sizeof(CellSpurs::EventPortMux));
	eventPortMux->spuPort   = spuPort;
	eventPortMux->eventPort = eventPort;
	eventPortMux->x08       = unknown;
}

/// Enable the system workload
s32 spursAddDefaultSystemWorkload(vm::ptr<CellSpurs> spurs, vm::ptr<const u8> swlPriority, u32 swlMaxSpu, u32 swlIsPreem)
{
	// TODO: Implement this
	return CELL_OK;
}

/// Destroy the SPURS SPU threads and thread group
s32 spursFinalizeSpu(vm::ptr<CellSpurs> spurs)
{
	if (spurs->flags & SAF_UNKNOWN_FLAG_7 || spurs->flags & SAF_UNKNOWN_FLAG_8)
	{
		while (true)
		{
			if (s32 rc = sys_spu_thread_group_join(spurs->spuTG, vm::null, vm::null))
			{
				throw EXCEPTION("sys_spu_thread_group_join() failed (0x%x)", rc);
			}

			if (s32 rc = sys_spu_thread_group_destroy(spurs->spuTG))
			{
				if (rc != CELL_EBUSY)
				{
					throw EXCEPTION("sys_spu_thread_group_destroy() failed (0x%x)", rc);
				}

				continue;
			}

			break;
		}
	}
	else
	{
		if (s32 rc = sys_spu_thread_group_destroy(spurs->spuTG))
		{
			return rc;
		}
	}

	if (s32 rc = sys_spu_image_close(spurs.ptr(&CellSpurs::spuImg)))
	{
		throw EXCEPTION("sys_spu_image_close() failed (0x%x)", rc);
	}

	return CELL_OK;
}

/// Stop the event helper thread
s32 spursStopEventHelper(PPUThread& ppu, vm::ptr<CellSpurs> spurs)
{
	if (spurs->ppu1 == 0xFFFFFFFF)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	if (sys_event_port_send(spurs->eventPort, 0, 1, 0) != CELL_OK)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	if (sys_ppu_thread_join(ppu, (u32)spurs->ppu1, vm::var<u64>(ppu)) != CELL_OK)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	spurs->ppu1 = 0xFFFFFFFF;

	if (s32 rc = sys_event_port_disconnect(spurs->eventPort))
	{
		throw EXCEPTION("sys_event_port_disconnect() failed (0x%x)", rc);
	}

	if (s32 rc = sys_event_port_destroy(spurs->eventPort))
	{
		throw EXCEPTION("sys_event_port_destroy() failed (0x%x)", rc);
	}

	if (s32 rc = spursDetachLv2EventQueue(spurs, spurs->spuPort, true /*spursCreated*/))
	{
		throw EXCEPTION("spursDetachLv2EventQueue() failed (0x%x)", rc);
	}

	if (s32 rc = sys_event_queue_destroy(spurs->eventQueue, SYS_EVENT_QUEUE_DESTROY_FORCE))
	{
		throw EXCEPTION("sys_event_queue_destroy() failed (0x%x)", rc);
	}

	return CELL_OK;
}

/// Signal to the SPURS handler thread
s32 spursSignalToHandlerThread(PPUThread& ppu, vm::ptr<CellSpurs> spurs)
{
	if (s32 rc = sys_lwmutex_lock(ppu, spurs.ptr(&CellSpurs::mutex), 0 /* forever */))
	{
		throw EXCEPTION("sys_lwmutex_lock() failed (0x%x)", rc);
	}

	if (s32 rc = sys_lwcond_signal(ppu, spurs.ptr(&CellSpurs::cond)))
	{
		throw EXCEPTION("sys_lwcond_signal() failed (0x%x)", rc);
	}

	if (s32 rc = sys_lwmutex_unlock(ppu, spurs.ptr(&CellSpurs::mutex)))
	{
		throw EXCEPTION("sys_lwmutex_unlock() failed (0x%x)", rc);
	}

	return CELL_OK;
}

/// Join the SPURS handler thread
s32 spursJoinHandlerThread(PPUThread& ppu, vm::ptr<CellSpurs> spurs)
{
	if (spurs->ppu0 == 0xFFFFFFFF)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	if (s32 rc = sys_ppu_thread_join(ppu, (u32)spurs->ppu0, vm::var<u64>(ppu)))
	{
		throw EXCEPTION("sys_ppu_thread_join() failed (0x%x)", rc);
	}

	spurs->ppu0 = 0xFFFFFFFF;
	return CELL_OK;
}

/// Initialise SPURS
s32 spursInit(
	PPUThread& ppu,
	vm::ptr<CellSpurs> spurs,
	u32 revision,
	u32 sdkVersion,
	s32 nSpus,
	s32 spuPriority,
	s32 ppuPriority,
	u32 flags, // SpursAttrFlags
	vm::cptr<char> prefix,
	u32 prefixSize,
	u32 container,
	vm::cptr<u8> swlPriority,
	u32 swlMaxSpu,
	u32 swlIsPreem)
{
	vm::var<u32>                      sem(ppu);
	vm::var<sys_semaphore_attribute_t>      semAttr(ppu);
	vm::var<sys_lwcond_attribute_t>         lwCondAttr(ppu);
	vm::var<sys_lwmutex_attribute_t>        lwMutexAttr(ppu);
	vm::var<u32>                      spuTgId(ppu);
	vm::var<char>                           spuTgName(ppu, 128);
	vm::var<sys_spu_thread_group_attribute> spuTgAttr(ppu);
	vm::var<sys_spu_thread_argument>        spuThArgs(ppu);
	vm::var<u32>                      spuThreadId(ppu);
	vm::var<sys_spu_thread_attribute>       spuThAttr(ppu);
	vm::var<char>                           spuThName(ppu, 128);

	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (prefixSize > CELL_SPURS_NAME_MAX_LENGTH)
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	if (sys_process_is_spu_lock_line_reservation_address(spurs.addr(), SYS_MEMORY_ACCESS_RIGHT_SPU_THR) != CELL_OK)
	{
		return CELL_SPURS_CORE_ERROR_PERM;
	}

	// Intialise SPURS context
	const bool isSecond = (flags & SAF_SECOND_VERSION) != 0;

	auto rollback = [=]
	{
		if (spurs->semPrv)
		{
			sys_semaphore_destroy((u32)spurs->semPrv);
		}

		for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++)
		{
			if (spurs->wklF1[i].sem)
			{
				sys_semaphore_destroy((u32)spurs->wklF1[i].sem);
			}

			if (isSecond)
			{
				if (spurs->wklF2[i].sem)
				{
					sys_semaphore_destroy((u32)spurs->wklF2[i].sem);
				}
			}
		}
	};

	memset(spurs.get_ptr(), 0, isSecond ? CELL_SPURS_SIZE2 : CELL_SPURS_SIZE);
	spurs->revision   = revision;
	spurs->sdkVersion = sdkVersion;
	spurs->ppu0       = 0xffffffffull;
	spurs->ppu1       = 0xffffffffull;
	spurs->flags      = flags;
	spurs->prefixSize = (u8)prefixSize;
	memcpy(spurs->prefix, prefix.get_ptr(), prefixSize);

	if (!isSecond)
	{
		spurs->wklEnabled.store(0xffff);
	}

	// Initialise trace
	spurs->sysSrvTrace = {};

	for (u32 i = 0; i < 8; i++)
	{
		spurs->sysSrvPreemptWklId[i] = -1;
	}

	// Import default system workload
	spurs->wklInfoSysSrv.addr.set(SPURS_IMG_ADDR_SYS_SRV_WORKLOAD);
	spurs->wklInfoSysSrv.size = 0x2200;
	spurs->wklInfoSysSrv.arg  = 0;
	spurs->wklInfoSysSrv.uniqueId.store(0xff);

	auto sys_semaphore_attribute_initialize = [](vm::ptr<sys_semaphore_attribute_t> attr)
	{
		attr->protocol = SYS_SYNC_PRIORITY;
		attr->pshared  = SYS_SYNC_NOT_PROCESS_SHARED;
		attr->ipc_key  = 0;
		attr->flags    = 0;
		attr->name[0]  = '\0';
	};

	// Create semaphores for each workload
	// TODO: Find out why these semaphores are needed
	sys_semaphore_attribute_initialize(semAttr);
	memcpy(semAttr->name, "_spuWkl", 8);
	for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++)
	{
		if (s32 rc = sys_semaphore_create(sem, semAttr, 0, 1))
		{
			return rollback(), rc;
		}

		spurs->wklF1[i].sem = *sem;

		if (isSecond)
		{
			if (s32 rc = sys_semaphore_create(sem, semAttr, 0, 1))
			{
				return rollback(), rc;
			}

			spurs->wklF2[i].sem = *sem;
		}
	}

	// Create semaphore
	// TODO: Figure out why this semaphore is needed
	memcpy(semAttr->name, "_spuPrv", 8);
	if (s32 rc = sys_semaphore_create(sem, semAttr, 0, 1))
	{
		return rollback(), rc;
	}

	spurs->semPrv      = *sem;

	spurs->unk11       = -1;
	spurs->unk12       = -1;
	spurs->unk13       = 0;
	spurs->nSpus       = nSpus;
	spurs->spuPriority = spuPriority;

	// Import SPURS kernel
	spurs->spuImg.type        = SYS_SPU_IMAGE_TYPE_USER;
	spurs->spuImg.addr        = vm::alloc(0x40000, vm::main);
	spurs->spuImg.entry_point = isSecond ? CELL_SPURS_KERNEL2_ENTRY_ADDR : CELL_SPURS_KERNEL1_ENTRY_ADDR;
	spurs->spuImg.nsegs       = 1;

	// Create a thread group for this SPURS context
	memcpy(spuTgName.get_ptr(), spurs->prefix, spurs->prefixSize);
	spuTgName[spurs->prefixSize] = '\0';
	strcat(spuTgName.get_ptr(), "CellSpursKernelGroup");

	auto sys_spu_thread_group_attribute_initialize = [](vm::ptr<sys_spu_thread_group_attribute> attr)
	{
		attr->name  = vm::null;
		attr->nsize = 0;
		attr->type  = SYS_SPU_THREAD_GROUP_TYPE_NORMAL;
	};

	sys_spu_thread_group_attribute_initialize(spuTgAttr);
	spuTgAttr->name  = spuTgName;
	spuTgAttr->nsize = (u32)strlen(spuTgAttr->name.get_ptr()) + 1;

	if (spurs->flags & SAF_UNKNOWN_FLAG_0)
	{
		spuTgAttr->type = 0x0C00 | SYS_SPU_THREAD_GROUP_TYPE_SYSTEM;
	}
	else if (flags & SAF_SPU_TGT_EXCLUSIVE_NON_CONTEXT)
	{
		spuTgAttr->type = SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT;
	}
	else
	{
		spuTgAttr->type = SYS_SPU_THREAD_GROUP_TYPE_NORMAL;
	}

	if (spurs->flags & SAF_SPU_MEMORY_CONTAINER_SET)
	{
		spuTgAttr->type |= SYS_SPU_THREAD_GROUP_TYPE_MEMORY_FROM_CONTAINER;
		spuTgAttr->ct    = container;
	}

	if (flags & SAF_UNKNOWN_FLAG_7)          spuTgAttr->type |= 0x0100 | SYS_SPU_THREAD_GROUP_TYPE_SYSTEM;
	if (flags & SAF_UNKNOWN_FLAG_8)          spuTgAttr->type |= 0x0C00 | SYS_SPU_THREAD_GROUP_TYPE_SYSTEM;
	if (flags & SAF_UNKNOWN_FLAG_9)          spuTgAttr->type |= 0x0800;
	if (flags & SAF_SYSTEM_WORKLOAD_ENABLED) spuTgAttr->type |= SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM;

	if (s32 rc = sys_spu_thread_group_create(spuTgId, nSpus, spuPriority, spuTgAttr))
	{
		sys_spu_image_close(spurs.ptr(&CellSpurs::spuImg));
		return rollback(), rc;
	}

	spurs->spuTG = *spuTgId;

	// Initialise all SPUs in the SPU thread group
	memcpy(spuThName.get_ptr(), spurs->prefix, spurs->prefixSize);
	spuThName[spurs->prefixSize] = '\0';
	strcat(spuThName.get_ptr(), "CellSpursKernel");

	spuThAttr->name                    = spuThName;
	spuThAttr->name_len                = (u32)strlen(spuThName.get_ptr()) + 2;
	spuThAttr->option                  = SYS_SPU_THREAD_OPTION_DEC_SYNC_TB_ENABLE;
	spuThName[spuThAttr->name_len - 1] = '\0';

	for (s32 num = 0; num < nSpus; num++)
	{
		spuThName[spuThAttr->name_len - 2] = '0' + num;
		spuThArgs->arg1                    = (u64)num << 32;
		spuThArgs->arg2                    = (u64)spurs.addr();

		if (s32 rc = sys_spu_thread_initialize(spuThreadId, spurs->spuTG, num, spurs.ptr(&CellSpurs::spuImg), spuThAttr, spuThArgs))
		{
			sys_spu_thread_group_destroy(spurs->spuTG);
			sys_spu_image_close(spurs.ptr(&CellSpurs::spuImg));
			return rollback(), rc;
		}

		const auto spuThread = idm::get<SPUThread>(spurs->spus[num] = *spuThreadId);

		// entry point cannot be initialized immediately because SPU LS will be rewritten by sys_spu_thread_group_start()
		spuThread->custom_task = [spurs](SPUThread& spu)
		{
			spu.RegisterHleFunction(spurs->spuImg.entry_point, spursKernelEntry);
			spu.fast_call(spurs->spuImg.entry_point);
		};
	}

	// Start the SPU printf server if required
	if (flags & SAF_SPU_PRINTF_ENABLED)
	{
		// spu_printf: attach group
		if (!g_spu_printf_agcb || g_spu_printf_agcb(ppu, spurs->spuTG) != CELL_OK)
		{
			// remove flag if failed
			spurs->flags &= ~SAF_SPU_PRINTF_ENABLED;
		}
	}

	const auto lwMutex = spurs.ptr(&CellSpurs::mutex);
	const auto lwCond  = spurs.ptr(&CellSpurs::cond);

	auto sys_lwmutex_attribute_initialize = [](vm::ptr<sys_lwmutex_attribute_t> attr)
	{
		attr->protocol  = SYS_SYNC_PRIORITY;
		attr->recursive = SYS_SYNC_NOT_RECURSIVE;
		attr->name[0]   = '\0';
	};

	// Create a mutex to protect access to SPURS handler thread data
	sys_lwmutex_attribute_initialize(lwMutexAttr);
	memcpy(lwMutexAttr->name, "_spuPrv", 8);
	if (s32 rc = sys_lwmutex_create(lwMutex, lwMutexAttr))
	{
		spursFinalizeSpu(spurs);
		return rollback(), rc;
	}

	// Create condition variable to signal the SPURS handler thread
	memcpy(lwCondAttr->name, "_spuPrv", 8);
	if (s32 rc = sys_lwcond_create(lwCond, lwMutex, lwCondAttr))
	{
		sys_lwmutex_destroy(ppu, lwMutex);
		spursFinalizeSpu(spurs);
		return rollback(), rc;
	}

	spurs->flags1 = (flags & SAF_EXIT_IF_NO_WORK ? SF1_EXIT_IF_NO_WORK : 0) | (isSecond ? SF1_32_WORKLOADS : 0);
	spurs->wklFlagReceiver.store(0xff);
	spurs->wklFlag.flag.store(-1);
	spurs->handlerDirty.store(0);
	spurs->handlerWaiting.store(0);
	spurs->handlerExiting.store(0);
	spurs->ppuPriority = ppuPriority;

	// Create the SPURS event helper thread
	if (s32 rc = spursCreateSpursEventHelper(ppu, spurs, ppuPriority))
	{
		sys_lwcond_destroy(lwCond);
		sys_lwmutex_destroy(ppu, lwMutex);
		spursFinalizeSpu(spurs);
		return rollback(), rc;
	}

	// Create the SPURS handler thread
	if (s32 rc = spursCreateHandler(spurs, ppuPriority))
	{
		spursStopEventHelper(ppu, spurs);
		sys_lwcond_destroy(lwCond);
		sys_lwmutex_destroy(ppu, lwMutex);
		spursFinalizeSpu(spurs);
		return rollback(), rc;
	}

	// Enable SPURS exception handler
	if (s32 rc = cellSpursEnableExceptionEventHandler(spurs, true /*enable*/))
	{
		spursSignalToHandlerThread(ppu, spurs);
		spursJoinHandlerThread(ppu, spurs);
		spursStopEventHelper(ppu, spurs);
		sys_lwcond_destroy(lwCond);
		sys_lwmutex_destroy(ppu, lwMutex);
		spursFinalizeSpu(spurs);
		return rollback(), rc;
	}

	spurs->traceBuffer = vm::null;
	// TODO: Register libprof for user trace 

	// Initialise the event port multiplexor
	spursInitialiseEventPortMux(spurs.ptr(&CellSpurs::eventPortMux), spurs->spuPort, spurs->eventPort, 3);

	// Enable the default system workload if required
	if (flags & SAF_SYSTEM_WORKLOAD_ENABLED)
	{
		if (s32 rc = spursAddDefaultSystemWorkload(spurs, swlPriority, swlMaxSpu, swlIsPreem))
		{
			throw EXCEPTION("spursAddDefaultSystemWorkload() failed (0x%x)", rc);
		}
		
		return CELL_OK;
	}
	else if (flags & SAF_EXIT_IF_NO_WORK)
	{
		return cellSpursWakeUp(ppu, spurs);
	}

	return CELL_OK;	
}

/// Initialise SPURS
s32 cellSpursInitialize(PPUThread& ppu, vm::ptr<CellSpurs> spurs, s32 nSpus, s32 spuPriority, s32 ppuPriority, b8 exitIfNoWork)
{
	cellSpurs.Warning("cellSpursInitialize(spurs=*0x%x, nSpus=%d, spuPriority=%d, ppuPriority=%d, exitIfNoWork=%d)", spurs, nSpus, spuPriority, ppuPriority, exitIfNoWork);

	return spursInit(ppu, spurs, 0, 0, nSpus, spuPriority, ppuPriority, exitIfNoWork ? SAF_EXIT_IF_NO_WORK : SAF_NONE, vm::null, 0, 0, vm::null, 0, 0);
}

/// Initialise SPURS
s32 cellSpursInitializeWithAttribute(PPUThread& ppu, vm::ptr<CellSpurs> spurs, vm::cptr<CellSpursAttribute> attr)
{
	cellSpurs.Warning("cellSpursInitializeWithAttribute(spurs=*0x%x, attr=*0x%x)", spurs, attr);

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (attr->revision > 2)
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	return spursInit(
		ppu,
		spurs,
		attr->revision,
		attr->sdkVersion,
		attr->nSpus,
		attr->spuPriority,
		attr->ppuPriority,
		attr->flags | (attr->exitIfNoWork ? SAF_EXIT_IF_NO_WORK : 0),
		attr.ptr(&CellSpursAttribute::prefix, 0),
		attr->prefixSize,
		attr->container,
		attr.ptr(&CellSpursAttribute::swlPriority, 0),
		attr->swlMaxSpu,
		attr->swlIsPreem);
}

/// Initialise SPURS
s32 cellSpursInitializeWithAttribute2(PPUThread& ppu, vm::ptr<CellSpurs> spurs, vm::cptr<CellSpursAttribute> attr)
{
	cellSpurs.Warning("cellSpursInitializeWithAttribute2(spurs=*0x%x, attr=*0x%x)", spurs, attr);

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (attr->revision > 2)
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	return spursInit(
		ppu,
		spurs,
		attr->revision,
		attr->sdkVersion,
		attr->nSpus,
		attr->spuPriority,
		attr->ppuPriority,
		attr->flags | (attr->exitIfNoWork ? SAF_EXIT_IF_NO_WORK : 0) | SAF_SECOND_VERSION,
		attr.ptr(&CellSpursAttribute::prefix, 0),
		attr->prefixSize,
		attr->container,
		attr.ptr(&CellSpursAttribute::swlPriority, 0),
		attr->swlMaxSpu,
		attr->swlIsPreem);
}

/// Initialise SPURS attribute
s32 _cellSpursAttributeInitialize(vm::ptr<CellSpursAttribute> attr, u32 revision, u32 sdkVersion, u32 nSpus, s32 spuPriority, s32 ppuPriority, b8 exitIfNoWork)
{
	cellSpurs.Warning("_cellSpursAttributeInitialize(attr=*0x%x, revision=%d, sdkVersion=0x%x, nSpus=%d, spuPriority=%d, ppuPriority=%d, exitIfNoWork=%d)",
		attr, revision, sdkVersion, nSpus, spuPriority, ppuPriority, exitIfNoWork);

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	memset(attr.get_ptr(), 0, sizeof(CellSpursAttribute));
	attr->revision     = revision;
	attr->sdkVersion   = sdkVersion;
	attr->nSpus        = nSpus;
	attr->spuPriority  = spuPriority;
	attr->ppuPriority  = ppuPriority;
	attr->exitIfNoWork = exitIfNoWork;
	return CELL_OK;
}

/// Set memory container ID for creating the SPU thread group
s32 cellSpursAttributeSetMemoryContainerForSpuThread(vm::ptr<CellSpursAttribute> attr, u32 container)
{
	cellSpurs.Warning("cellSpursAttributeSetMemoryContainerForSpuThread(attr=*0x%x, container=0x%x)", attr, container);

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (attr->flags & SAF_SPU_TGT_EXCLUSIVE_NON_CONTEXT)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	attr->container  = container;
	attr->flags     |= SAF_SPU_MEMORY_CONTAINER_SET;
	return CELL_OK;
}

/// Set the prefix for SPURS
s32 cellSpursAttributeSetNamePrefix(vm::ptr<CellSpursAttribute> attr, vm::cptr<char> prefix, u32 size)
{
	cellSpurs.Warning("cellSpursAttributeSetNamePrefix(attr=*0x%x, prefix=*0x%x, size=%d)", attr, prefix, size);

	if (!attr || !prefix)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (size > CELL_SPURS_NAME_MAX_LENGTH)
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	memcpy(attr->prefix, prefix.get_ptr(), size);
	attr->prefixSize = size;
	return CELL_OK;
}

/// Enable spu_printf()
s32 cellSpursAttributeEnableSpuPrintfIfAvailable(vm::ptr<CellSpursAttribute> attr)
{
	cellSpurs.Warning("cellSpursAttributeEnableSpuPrintfIfAvailable(attr=*0x%x)", attr);

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	attr->flags |= SAF_SPU_PRINTF_ENABLED;
	return CELL_OK;
}

/// Set the type of SPU thread group
s32 cellSpursAttributeSetSpuThreadGroupType(vm::ptr<CellSpursAttribute> attr, s32 type)
{
	cellSpurs.Warning("cellSpursAttributeSetSpuThreadGroupType(attr=*0x%x, type=%d)", attr, type);

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (type == SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT)
	{
		if (attr->flags & SAF_SPU_MEMORY_CONTAINER_SET)
		{
			return CELL_SPURS_CORE_ERROR_STAT;
		}
		attr->flags |= SAF_SPU_TGT_EXCLUSIVE_NON_CONTEXT; // set
	}
	else if (type == SYS_SPU_THREAD_GROUP_TYPE_NORMAL)
	{
		attr->flags &= ~SAF_SPU_TGT_EXCLUSIVE_NON_CONTEXT; // clear
	}
	else
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	return CELL_OK;
}

/// Enable the system workload
s32 cellSpursAttributeEnableSystemWorkload(vm::ptr<CellSpursAttribute> attr, vm::cptr<u8[8]> priority, u32 maxSpu, vm::cptr<b8[8]> isPreemptible)
{
	cellSpurs.Warning("cellSpursAttributeEnableSystemWorkload(attr=*0x%x, priority=*0x%x, maxSpu=%d, isPreemptible=*0x%x)", attr, priority, maxSpu, isPreemptible);

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	const u32 nSpus = attr->nSpus;

	if (!nSpus)
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	for (u32 i = 0; i < nSpus; i++)
	{
		if ((*priority)[i] == 1)
		{
			if (!maxSpu)
			{
				return CELL_SPURS_CORE_ERROR_INVAL;
			}

			if (nSpus == 1 || attr->exitIfNoWork)
			{
				return CELL_SPURS_CORE_ERROR_PERM;
			}

			if (attr->flags & SAF_SYSTEM_WORKLOAD_ENABLED)
			{
				return CELL_SPURS_CORE_ERROR_BUSY;
			}

			attr->flags |= SAF_SYSTEM_WORKLOAD_ENABLED; // set flag
			*(u64*)attr->swlPriority = *(u64*)*priority; // copy system workload priorities

			u32 isPreem = 0; // generate mask from isPreemptible values
			for (u32 j = 0; j < nSpus; j++)
			{
				if ((*isPreemptible)[j])
				{
					isPreem |= (1 << j);
				}
			}

			attr->swlMaxSpu  = maxSpu;  // write max spu for system workload
			attr->swlIsPreem = isPreem; // write isPreemptible mask
			return CELL_OK;
		}
	}

	return CELL_SPURS_CORE_ERROR_INVAL;
}

/// Release resources allocated for SPURS
s32 cellSpursFinalize(vm::ptr<CellSpurs> spurs)
{
	cellSpurs.Todo("cellSpursFinalize(spurs=*0x%x)", spurs);

	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (spurs->handlerExiting.load())
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	u32 wklEnabled = spurs->wklEnabled.load();

	if (spurs->flags1 & SF1_32_WORKLOADS)
	{
		wklEnabled &= 0xFFFF0000;
	}

	if (spurs->flags & SAF_SYSTEM_WORKLOAD_ENABLED)
	{
	}

	// TODO: Implement the rest of this function
	return CELL_OK;
}

/// Get the SPU thread group ID
s32 cellSpursGetSpuThreadGroupId(vm::ptr<CellSpurs> spurs, vm::ptr<u32> group)
{
	cellSpurs.Warning("cellSpursGetSpuThreadGroupId(spurs=*0x%x, group=*0x%x)", spurs, group);

	if (!spurs || !group)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	*group = spurs->spuTG;
	return CELL_OK;
}

// Get the number of SPU threads
s32 cellSpursGetNumSpuThread(vm::ptr<CellSpurs> spurs, vm::ptr<u32> nThreads)
{
	cellSpurs.Warning("cellSpursGetNumSpuThread(spurs=*0x%x, nThreads=*0x%x)", spurs, nThreads);

	if (!spurs || !nThreads)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	*nThreads = spurs->nSpus;
	return CELL_OK;
}

/// Get SPU thread ids
s32 cellSpursGetSpuThreadId(vm::ptr<CellSpurs> spurs, vm::ptr<u32> thread, vm::ptr<u32> nThreads)
{
	cellSpurs.Warning("cellSpursGetSpuThreadId(spurs=*0x%x, thread=*0x%x, nThreads=*0x%x)", spurs, thread, nThreads);

	if (!spurs || !thread || !nThreads)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	const u32 count = std::min<u32>(*nThreads, spurs->nSpus);

	for (u32 i = 0; i < count; i++)
	{
		thread[i] = spurs->spus[i];
	}

	*nThreads = count;
	return CELL_OK;
}

/// Set the maximum contention for a workload
s32 cellSpursSetMaxContention(vm::ptr<CellSpurs> spurs, u32 wid, u32 maxContention)
{
	cellSpurs.Warning("cellSpursSetMaxContention(spurs=*0x%x, wid=%d, maxContention=%d)", spurs, wid, maxContention);

	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (wid >= (spurs->flags1 & SF1_32_WORKLOADS ? CELL_SPURS_MAX_WORKLOAD2 : CELL_SPURS_MAX_WORKLOAD))
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	if ((spurs->wklEnabled.load() & (0x80000000u >> wid)) == 0)
	{
		return CELL_SPURS_CORE_ERROR_SRCH;
	}

	if (spurs->exception.data())
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	if (maxContention > CELL_SPURS_MAX_SPU)
	{
		maxContention = CELL_SPURS_MAX_SPU;
	}

	spurs->wklMaxContention[wid % CELL_SPURS_MAX_WORKLOAD].atomic_op([spurs, wid, maxContention](u8& value)
	{
		value &= wid < CELL_SPURS_MAX_WORKLOAD ? 0xF0 : 0x0F;
		value |= wid < CELL_SPURS_MAX_WORKLOAD ? maxContention : maxContention << 4;
	});

	return CELL_OK;
}

/// Set the priority of a workload on each SPU
s32 cellSpursSetPriorities(vm::ptr<CellSpurs> spurs, u32 wid, vm::cptr<u8> priorities)
{
	cellSpurs.Warning("cellSpursSetPriorities(spurs=*0x%x, wid=%d, priorities=*0x%x)", spurs, wid, priorities);

	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (wid >= (spurs->flags1 & SF1_32_WORKLOADS ? CELL_SPURS_MAX_WORKLOAD2 : CELL_SPURS_MAX_WORKLOAD))
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	if ((spurs->wklEnabled.load() & (0x80000000u >> wid)) == 0)
	{
		return CELL_SPURS_CORE_ERROR_SRCH;
	}

	if (spurs->exception.data())
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	if (spurs->flags & SAF_SYSTEM_WORKLOAD_ENABLED)
	{
		// TODO: Implement this
	}

	u64 prio = 0;
	for (s32 i = 0; i < CELL_SPURS_MAX_SPU; i++)
	{
		if (priorities[i] >= CELL_SPURS_MAX_PRIORITY)
		{
			return CELL_SPURS_CORE_ERROR_INVAL;
		}

		prio |=  priorities[i];
		prio <<= 8;
	}

	auto& wklInfo = wid < CELL_SPURS_MAX_WORKLOAD ? spurs->wklInfo1[wid] : spurs->wklInfo2[wid];
	*((be_t<u64>*)wklInfo.priority) = prio;

	spurs->sysSrvMsgUpdateWorkload.store(0xFF);
	spurs->sysSrvMessage.store(0xFF);
	return CELL_OK;
}

/// Set the priority of a workload for the specified SPU
s32 cellSpursSetPriority(vm::ptr<CellSpurs> spurs, u32 wid, u32 spuId, u32 priority)
{
	cellSpurs.Todo("cellSpursSetPriority(spurs=*0x%x, wid=%d, spuId=%d, priority=%d)", spurs, wid, spuId, priority);
	return CELL_OK;
}

/// Set preemption victim SPU
s32 cellSpursSetPreemptionVictimHints(vm::ptr<CellSpurs> spurs, vm::cptr<b8> isPreemptible)
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

/// Attach an LV2 event queue to a SPURS instance
s32 cellSpursAttachLv2EventQueue(PPUThread& ppu, vm::ptr<CellSpurs> spurs, u32 queue, vm::ptr<u8> port, s32 isDynamic)
{
	cellSpurs.Warning("cellSpursAttachLv2EventQueue(spurs=*0x%x, queue=0x%x, port=*0x%x, isDynamic=%d)", spurs, queue, port, isDynamic);

	return spursAttachLv2EventQueue(ppu, spurs, queue, port, isDynamic, false /*spursCreated*/);
}

/// Detach an LV2 event queue from a SPURS instance
s32 cellSpursDetachLv2EventQueue(vm::ptr<CellSpurs> spurs, u8 port)
{
	cellSpurs.Warning("cellSpursDetachLv2EventQueue(spurs=*0x%x, port=%d)", spurs, port);

	return spursDetachLv2EventQueue(spurs, port, false /*spursCreated*/);
}

/// Enable the SPU exception event handler
s32 cellSpursEnableExceptionEventHandler(vm::ptr<CellSpurs> spurs, b8 flag)
{
	cellSpurs.Warning("cellSpursEnableExceptionEventHandler(spurs=*0x%x, flag=%d)", spurs, flag);

	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	s32  rc          = CELL_OK;
	auto oldEnableEH = spurs->enableEH.exchange(flag ? 1u : 0u);
	if (flag)
	{
		if (oldEnableEH == 0)
		{
			rc = sys_spu_thread_group_connect_event(spurs->spuTG, spurs->eventQueue, SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION);
		}
	}
	else
	{
		if (oldEnableEH == 1)
		{
			rc = sys_spu_thread_group_disconnect_event(spurs->eventQueue, SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION);
		}
	}

	return rc;
}

/// Set the global SPU exception event handler
s32 cellSpursSetGlobalExceptionEventHandler(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursGlobalExceptionEventHandler> eaHandler, vm::ptr<void> arg)
{
	cellSpurs.Warning("cellSpursSetGlobalExceptionEventHandler(spurs=*0x%x, eaHandler=*0x%x, arg=*0x%x)", spurs, eaHandler, arg);

	if (!spurs || !eaHandler)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (spurs->exception.data())
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	auto handler = spurs->globalSpuExceptionHandler.compare_and_swap(0, 1);
	if (handler)
	{
		return CELL_SPURS_CORE_ERROR_BUSY;
	}

	spurs->globalSpuExceptionHandlerArgs = arg.addr();
	spurs->globalSpuExceptionHandler.exchange(eaHandler.addr());
	return CELL_OK;
}


/// Remove the global SPU exception event handler
s32 cellSpursUnsetGlobalExceptionEventHandler(vm::ptr<CellSpurs> spurs)
{
	cellSpurs.Warning("cellSpursUnsetGlobalExceptionEventHandler(spurs=*0x%x)", spurs);

	spurs->globalSpuExceptionHandlerArgs = 0;
	spurs->globalSpuExceptionHandler.exchange(0);
	return CELL_OK;
}

/// Get internal information of a SPURS instance
s32 cellSpursGetInfo(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursInfo> info)
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

//----------------------------------------------------------------------------
// SPURS SPU GUID functions
//----------------------------------------------------------------------------

/// Get the SPU GUID from a .SpuGUID section
s32 cellSpursGetSpuGuid()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

//----------------------------------------------------------------------------
// SPURS trace functions
//----------------------------------------------------------------------------

/// Signal SPUs to update trace status
void spursTraceStatusUpdate(PPUThread& ppu, vm::ptr<CellSpurs> spurs)
{
	u8 init;

	spurs->sysSrvTrace.atomic_op([spurs, &init](CellSpurs::SrvTraceSyncVar& data)
	{
		if ((init = data.sysSrvTraceInitialised))
		{
			data.sysSrvNotifyUpdateTraceComplete = 1;
			data.sysSrvMsgUpdateTrace            = (1 << spurs->nSpus) - 1;
		}
	});

	if (init)
	{
		spurs->sysSrvMessage.store(0xFF);

		if (s32 rc = sys_semaphore_wait(ppu, (u32)spurs->semPrv, 0))
		{
			throw EXCEPTION("sys_semaphore_wait() failed (0x%x)", rc);
		}
	}
}

/// Initialize SPURS trace
s32 spursTraceInitialize(PPUThread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTraceInfo> buffer, u32 size, u32 mode, u32 updateStatus)
{
	if (!spurs || !buffer)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned() || !buffer.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (size < sizeof32(CellSpursTraceInfo) || mode & ~(CELL_SPURS_TRACE_MODE_FLAG_MASK))
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	if (spurs->traceBuffer)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	spurs->traceDataSize = size - sizeof32(CellSpursTraceInfo);
	for (u32 i = 0; i < 8; i++)
	{
		buffer->spuThread[i] = spurs->spus[i];
		buffer->count[i]     = 0;
	}

	buffer->spuThreadGroup = spurs->spuTG;
	buffer->numSpus        = spurs->nSpus;
	spurs->traceBuffer.set(buffer.addr() | (mode & CELL_SPURS_TRACE_MODE_FLAG_WRAP_BUFFER ? 1 : 0));
	spurs->traceMode     = mode;

	u32 spuTraceDataCount = (u32)((spurs->traceDataSize / sizeof32(CellSpursTracePacket)) / spurs->nSpus);
	for (u32 i = 0, j = 8; i < 6; i++)
	{
		spurs->traceStartIndex[i] = j;
		j += spuTraceDataCount;
	}

	spurs->sysSrvTraceControl = 0;
	if (updateStatus)
	{
		spursTraceStatusUpdate(ppu, spurs);
	}

	return CELL_OK;
}

/// Initialize SPURS trace
s32 cellSpursTraceInitialize(PPUThread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTraceInfo> buffer, u32 size, u32 mode)
{
	cellSpurs.Warning("cellSpursTraceInitialize(spurs=*0x%x, buffer=*0x%x, size=0x%x, mode=0x%x)", spurs, buffer, size, mode);

	if (spursIsLibProfLoaded())
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	return spursTraceInitialize(ppu, spurs, buffer, size, mode, 1);
}

/// Finalize SPURS trace
s32 cellSpursTraceFinalize(PPUThread& ppu, vm::ptr<CellSpurs> spurs)
{
	cellSpurs.Warning("cellSpursTraceFinalize(spurs=*0x%x)", spurs);

	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs->traceBuffer)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	spurs->sysSrvTraceControl = 0;
	spurs->traceMode          = 0;
	spurs->traceBuffer        = vm::null;
	spursTraceStatusUpdate(ppu, spurs);
	return CELL_OK;
}

/// Start SPURS trace
s32 spursTraceStart(PPUThread& ppu, vm::ptr<CellSpurs> spurs, u32 updateStatus)
{
	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs->traceBuffer)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	spurs->sysSrvTraceControl = 1;
	if (updateStatus)
	{
		spursTraceStatusUpdate(ppu, spurs);
	}

	return CELL_OK;
}

/// Start SPURS trace
s32 cellSpursTraceStart(PPUThread& ppu, vm::ptr<CellSpurs> spurs)
{
	cellSpurs.Warning("cellSpursTraceStart(spurs=*0x%x)", spurs);

	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	return spursTraceStart(ppu, spurs, spurs->traceMode & CELL_SPURS_TRACE_MODE_FLAG_SYNCHRONOUS_START_STOP);
}

/// Stop SPURS trace
s32 spursTraceStop(PPUThread& ppu, vm::ptr<CellSpurs> spurs, u32 updateStatus)
{
	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs->traceBuffer)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	spurs->sysSrvTraceControl = 2;
	if (updateStatus)
	{
		spursTraceStatusUpdate(ppu, spurs);
	}

	return CELL_OK;
}

/// Stop SPURS trace
s32 cellSpursTraceStop(PPUThread& ppu, vm::ptr<CellSpurs> spurs)
{
	cellSpurs.Warning("cellSpursTraceStop(spurs=*0x%x)", spurs);

	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	return spursTraceStop(ppu, spurs, spurs->traceMode & CELL_SPURS_TRACE_MODE_FLAG_SYNCHRONOUS_START_STOP);
}

//----------------------------------------------------------------------------
// SPURS policy module functions
//----------------------------------------------------------------------------

/// Initialize attributes of a workload
s32 _cellSpursWorkloadAttributeInitialize(vm::ptr<CellSpursWorkloadAttribute> attr, u32 revision, u32 sdkVersion, vm::cptr<void> pm, u32 size, u64 data, vm::cptr<u8[8]> priority, u32 minCnt, u32 maxCnt)
{
	cellSpurs.Warning("_cellSpursWorkloadAttributeInitialize(attr=*0x%x, revision=%d, sdkVersion=0x%x, pm=*0x%x, size=0x%x, data=0x%llx, priority=*0x%x, minCnt=0x%x, maxCnt=0x%x)",
		attr, revision, sdkVersion, pm, size, data, priority, minCnt, maxCnt);

	if (!attr)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (!pm)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (pm % 16)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (minCnt == 0 || *(u64*)*priority & 0xf0f0f0f0f0f0f0f0ull) // check if some priority > 15
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}
	
	memset(attr.get_ptr(), 0, sizeof(CellSpursWorkloadAttribute));
	attr->revision = revision;
	attr->sdkVersion = sdkVersion;
	attr->pm = pm;
	attr->size = size;
	attr->data = data;
	*(u64*)attr->priority = *(u64*)*priority;
	attr->minContention = minCnt;
	attr->maxContention = maxCnt;
	return CELL_OK;
}

/// Set the name of a workload
s32 cellSpursWorkloadAttributeSetName(vm::ptr<CellSpursWorkloadAttribute> attr, vm::cptr<char> nameClass, vm::cptr<char> nameInstance)
{
	cellSpurs.Warning("cellSpursWorkloadAttributeSetName(attr=*0x%x, nameClass=*0x%x, nameInstance=*0x%x)", attr, nameClass, nameInstance);

	if (!attr)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	attr->nameClass = nameClass;
	attr->nameInstance = nameInstance;
	return CELL_OK;
}

/// Set a hook function for shutdown completion event of a workload
s32 cellSpursWorkloadAttributeSetShutdownCompletionEventHook(vm::ptr<CellSpursWorkloadAttribute> attr, vm::ptr<CellSpursShutdownCompletionEventHook> hook, vm::ptr<void> arg)
{
	cellSpurs.Warning("cellSpursWorkloadAttributeSetShutdownCompletionEventHook(attr=*0x%x, hook=*0x%x, arg=*0x%x)", attr, hook, arg);

	if (!attr || !hook)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	attr->hook = hook;
	attr->hookArg = arg;
	return CELL_OK;
}

/// Add workload
s32 spursAddWorkload(
	vm::ptr<CellSpurs> spurs,
	vm::ptr<u32> wid,
	vm::cptr<void> pm,
	u32 size,
	u64 data,
	const u8 priorityTable[],
	u32 minContention,
	u32 maxContention,
	vm::cptr<char> nameClass,
	vm::cptr<char> nameInstance,
	vm::ptr<CellSpursShutdownCompletionEventHook> hook,
	vm::ptr<void> hookArg)
{
	if (!spurs || !wid || !pm)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned() || pm % 16)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (minContention == 0 || *(u64*)priorityTable & 0xf0f0f0f0f0f0f0f0ull) // check if some priority > 15
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}

	if (spurs->exception.data())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}
	
	u32 wnum;
	const u32 wmax = spurs->flags1 & SF1_32_WORKLOADS ? 0x20u : 0x10u; // TODO: check if can be changed
	spurs->wklEnabled.atomic_op([spurs, wmax, &wnum](be_t<u32>& value)
	{
		wnum = cntlz32(~(u32)value); // found empty position
		if (wnum < wmax)
		{
			value |= (u32)(0x80000000ull >> wnum); // set workload bit
		}
	});

	*wid = wnum; // store workload id
	if (wnum >= wmax)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_AGAIN;
	}

	u32 index = wnum & 0xf;
	if (wnum <= 15)
	{
		assert((spurs->wklCurrentContention[wnum] & 0xf) == 0);
		assert((spurs->wklPendingContention[wnum] & 0xf) == 0);
		spurs->wklState1[wnum].store(1);
		spurs->wklStatus1[wnum] = 0;
		spurs->wklEvent1[wnum].store(0);
		spurs->wklInfo1[wnum].addr = pm;
		spurs->wklInfo1[wnum].arg = data;
		spurs->wklInfo1[wnum].size = size;

		for (u32 i = 0; i < 8; i++)
		{
			spurs->wklInfo1[wnum].priority[i] = priorityTable[i];
		}

		spurs->wklH1[wnum].nameClass = nameClass;
		spurs->wklH1[wnum].nameInstance = nameInstance;
		memset(spurs->wklF1[wnum].unk0, 0, 0x20); // clear struct preserving semaphore id
		memset(&spurs->wklF1[wnum].x28, 0, 0x58);

		if (hook)
		{
			spurs->wklF1[wnum].hook = hook;
			spurs->wklF1[wnum].hookArg = hookArg;
			spurs->wklEvent1[wnum] |= 2;
		}

		if ((spurs->flags1 & SF1_32_WORKLOADS) == 0)
		{
			spurs->wklIdleSpuCountOrReadyCount2[wnum].store(0);
			spurs->wklMinContention[wnum] = minContention > 8 ? 8 : minContention;
		}

		spurs->wklReadyCount1[wnum].store(0);
	}
	else
	{
		assert((spurs->wklCurrentContention[index] & 0xf0) == 0);
		assert((spurs->wklPendingContention[index] & 0xf0) == 0);
		spurs->wklState2[index].store(1);
		spurs->wklStatus2[index] = 0;
		spurs->wklEvent2[index].store(0);
		spurs->wklInfo2[index].addr = pm;
		spurs->wklInfo2[index].arg = data;
		spurs->wklInfo2[index].size = size;

		for (u32 i = 0; i < 8; i++)
		{
			spurs->wklInfo2[index].priority[i] = priorityTable[i];
		}

		spurs->wklH2[index].nameClass = nameClass;
		spurs->wklH2[index].nameInstance = nameInstance;
		memset(spurs->wklF2[index].unk0, 0, 0x20); // clear struct preserving semaphore id
		memset(&spurs->wklF2[index].x28, 0, 0x58);

		if (hook)
		{
			spurs->wklF2[index].hook = hook;
			spurs->wklF2[index].hookArg = hookArg;
			spurs->wklEvent2[index] |= 2;
		}

		spurs->wklIdleSpuCountOrReadyCount2[wnum].store(0);
	}

	if (wnum <= 15)
	{
		spurs->wklMaxContention[wnum].atomic_op([maxContention](u8& v)
		{
			v &= ~0xf;
			v |= (maxContention > 8 ? 8 : maxContention);
		});
		spurs->wklSignal1._and_not(0x8000 >> index); // clear bit in wklFlag1
	}
	else
	{
		spurs->wklMaxContention[index].atomic_op([maxContention](u8& v)
		{
			v &= ~0xf0;
			v |= (maxContention > 8 ? 8 : maxContention) << 4;
		});
		spurs->wklSignal2._and_not(0x8000 >> index); // clear bit in wklFlag2
	}

	spurs->wklFlagReceiver.compare_and_swap(wnum, 0xff);

	u32 res_wkl;
	CellSpurs::WorkloadInfo& wkl = wnum <= 15 ? spurs->wklInfo1[wnum] : spurs->wklInfo2[wnum & 0xf];
	spurs->wklMskB.atomic_op([spurs, &wkl, wnum, &res_wkl](be_t<u32>& v)
	{
		const u32 mask = v & ~(0x80000000u >> wnum);
		res_wkl = 0;

		for (u32 i = 0, m = 0x80000000, k = 0; i < 32; i++, m >>= 1)
		{
			if (mask & m)
			{
				CellSpurs::WorkloadInfo& current = i <= 15 ? spurs->wklInfo1[i] : spurs->wklInfo2[i & 0xf];
				if (current.addr == wkl.addr)
				{
					// if a workload with identical policy module found
					res_wkl = current.uniqueId.load();
					break;
				}
				else
				{
					k |= 0x80000000 >> current.uniqueId.load();
					res_wkl = cntlz32(~k);
				}
			}
		}

		wkl.uniqueId.exchange((u8)res_wkl);
		v = mask | (0x80000000u >> wnum);
	});
	assert(res_wkl <= 31);

	spurs->wklState(wnum).exchange(2);
	spurs->sysSrvMsgUpdateWorkload.exchange(0xff);
	spurs->sysSrvMessage.exchange(0xff);
	return CELL_OK;
}

/// Add workload
s32 cellSpursAddWorkload(vm::ptr<CellSpurs> spurs, vm::ptr<u32> wid, vm::cptr<void> pm, u32 size, u64 data, vm::cptr<u8[8]> priority, u32 minCnt, u32 maxCnt)
{
	cellSpurs.Warning("cellSpursAddWorkload(spurs=*0x%x, wid=*0x%x, pm=*0x%x, size=0x%x, data=0x%llx, priority=*0x%x, minCnt=0x%x, maxCnt=0x%x)",
		spurs, wid, pm, size, data, priority, minCnt, maxCnt);

	return spursAddWorkload(spurs, wid, pm, size, data, *priority, minCnt, maxCnt, vm::null, vm::null, vm::null, vm::null);
}

/// Add workload
s32 cellSpursAddWorkloadWithAttribute(vm::ptr<CellSpurs> spurs, vm::ptr<u32> wid, vm::cptr<CellSpursWorkloadAttribute> attr)
{
	cellSpurs.Warning("cellSpursAddWorkloadWithAttribute(spurs=*0x%x, wid=*0x%x, attr=*0x%x)", spurs, wid, attr);

	if (!attr)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (attr->revision != 1)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}

	return spursAddWorkload(spurs, wid, attr->pm, attr->size, attr->data, attr->priority, attr->minContention, attr->maxContention, attr->nameClass, attr->nameInstance, attr->hook, attr->hookArg);
}

/// Request workload shutdown
s32 cellSpursShutdownWorkload()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

/// Wait for workload shutdown
s32 cellSpursWaitForWorkloadShutdown()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

/// Remove workload
s32 cellSpursRemoveWorkload()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

/// Activate the SPURS kernel
s32 cellSpursWakeUp(PPUThread& ppu, vm::ptr<CellSpurs> spurs)
{
	cellSpurs.Warning("cellSpursWakeUp(spurs=*0x%x)", spurs);

	if (!spurs)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (spurs->exception.data())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	spurs->handlerDirty.exchange(1);

	if (spurs->handlerWaiting.load())
	{
		spursSignalToHandlerThread(ppu, spurs);
	}
	
	return CELL_OK;
}

/// Send a workload signal
s32 cellSpursSendWorkloadSignal(vm::ptr<CellSpurs> spurs, u32 wid)
{
	cellSpurs.Warning("cellSpursSendWorkloadSignal(spurs=*0x%x, wid=%d)", spurs, wid);

	if (!spurs)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (wid >= CELL_SPURS_MAX_WORKLOAD2 || (wid >= CELL_SPURS_MAX_WORKLOAD && (spurs->flags1 & SF1_32_WORKLOADS) == 0))
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}

	if ((spurs->wklEnabled.load() & (0x80000000u >> wid)) == 0)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_SRCH;
	}

	if (spurs->exception)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	if (spurs->wklState(wid).load() != SPURS_WKL_STATE_RUNNABLE)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	if (wid >= CELL_SPURS_MAX_WORKLOAD)
	{
		spurs->wklSignal2 |= 0x8000 >> (wid & 0x0F);
	}
	else
	{
		spurs->wklSignal1 |= 0x8000 >> wid;
	}

	return CELL_OK;
}

/// Get the address of the workload flag
s32 cellSpursGetWorkloadFlag(vm::ptr<CellSpurs> spurs, vm::pptr<CellSpursWorkloadFlag> flag)
{
	cellSpurs.Warning("cellSpursGetWorkloadFlag(spurs=*0x%x, flag=**0x%x)", spurs, flag);

	if (!spurs || !flag)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	*flag = spurs.ptr(&CellSpurs::wklFlag);
	return CELL_OK;
}

/// Set ready count
s32 cellSpursReadyCountStore(vm::ptr<CellSpurs> spurs, u32 wid, u32 value)
{
	cellSpurs.Warning("cellSpursReadyCountStore(spurs=*0x%x, wid=%d, value=0x%x)", spurs, wid, value);

	if (!spurs)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (wid >= (spurs->flags1 & SF1_32_WORKLOADS ? 0x20u : 0x10u) || value > 0xff)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}

	if ((spurs->wklEnabled.load() & (0x80000000u >> wid)) == 0)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_SRCH;
	}

	if (spurs->exception.data() || spurs->wklState(wid).load() != 2)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	if (wid < CELL_SPURS_MAX_WORKLOAD)
	{
		spurs->wklReadyCount1[wid].exchange((u8)value);
	}
	else
	{
		spurs->wklIdleSpuCountOrReadyCount2[wid].exchange((u8)value);
	}

	return CELL_OK;
}

/// Swap ready count
s32 cellSpursReadyCountSwap()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

/// Compare and swap ready count
s32 cellSpursReadyCountCompareAndSwap()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

/// Increase or decrease ready count
s32 cellSpursReadyCountAdd()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

/// Get workload's data to be passed to policy module
s32 cellSpursGetWorkloadData(vm::ptr<CellSpurs> spurs, vm::ptr<u64> data, u32 wid)
{
	cellSpurs.Warning("cellSpursGetWorkloadData(spurs=*0x%x, data=*0x%x, wid=%d)", spurs, data, wid);

	if (!spurs || !data)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (wid >= CELL_SPURS_MAX_WORKLOAD2 || (wid >= CELL_SPURS_MAX_WORKLOAD && (spurs->flags1 & SF1_32_WORKLOADS) == 0))
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}

	if ((spurs->wklEnabled.load() & (0x80000000u >> wid)) == 0)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_SRCH;
	}

	if (spurs->exception)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	if (wid >= CELL_SPURS_MAX_WORKLOAD)
	{
		*data = spurs->wklInfo2[wid & 0x0F].arg;
	}
	else
	{
		*data = spurs->wklInfo1[wid].arg;
	}

	return CELL_OK;
}

/// Get workload information
s32 cellSpursGetWorkloadInfo()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

/// Set the SPU exception event handler
s32 cellSpursSetExceptionEventHandler()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

/// Disable the SPU exception event handler
s32 cellSpursUnsetExceptionEventHandler()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

/// Set/unset the recipient of the workload flag
s32 _cellSpursWorkloadFlagReceiver(vm::ptr<CellSpurs> spurs, u32 wid, u32 is_set)
{
	cellSpurs.Warning("_cellSpursWorkloadFlagReceiver(spurs=*0x%x, wid=%d, is_set=%d)", spurs, wid, is_set);

	if (!spurs)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (wid >= (spurs->flags1 & SF1_32_WORKLOADS ? 0x20u : 0x10u))
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}

	if ((spurs->wklEnabled.load() & (0x80000000u >> wid)) == 0)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_SRCH;
	}

	if (spurs->exception.data())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	_mm_mfence();

	if (s32 res = spurs->wklFlag.flag.atomic_op([spurs, wid, is_set](be_t<u32>& flag) -> s32
	{
		if (is_set)
		{
			if (spurs->wklFlagReceiver.load() != 0xff)
			{
				return CELL_SPURS_POLICY_MODULE_ERROR_BUSY;
			}
		}
		else
		{
			if (spurs->wklFlagReceiver.load() != wid)
			{
				return CELL_SPURS_POLICY_MODULE_ERROR_PERM;
			}
		}
		flag = -1;
		return 0;
	}))
	{
		return res;
	}

	spurs->wklFlagReceiver.atomic_op([wid, is_set](u8& FR)
	{
		if (is_set)
		{
			if (FR == 0xff)
			{
				FR = (u8)wid;
			}
		}
		else
		{
			if (FR == wid)
			{
				FR = 0xff;
			}
		}
	});
	return CELL_OK;
}

/// Set/unset the recipient of the workload flag
s32 _cellSpursWorkloadFlagReceiver2()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

/// Request assignment of idle SPUs
s32 cellSpursRequestIdleSpu()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

//----------------------------------------------------------------------------
// SPURS event flag functions
//----------------------------------------------------------------------------

/// Initialize a SPURS event flag
s32 _cellSpursEventFlagInitialize(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursEventFlag> eventFlag, u32 flagClearMode, u32 flagDirection)
{
	cellSpurs.Warning("_cellSpursEventFlagInitialize(spurs=*0x%x, taskset=*0x%x, eventFlag=*0x%x, flagClearMode=%d, flagDirection=%d)", spurs, taskset, eventFlag, flagClearMode, flagDirection);

	if ((!taskset && !spurs) || !eventFlag)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned() || !taskset.aligned() || !eventFlag.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (taskset && taskset->wid >= CELL_SPURS_MAX_WORKLOAD2)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	if (flagDirection > CELL_SPURS_EVENT_FLAG_LAST || flagClearMode > CELL_SPURS_EVENT_FLAG_CLEAR_LAST)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	memset(eventFlag.get_ptr(), 0, sizeof(CellSpursEventFlag));
	eventFlag->direction = flagDirection;
	eventFlag->clearMode = flagClearMode;
	eventFlag->spuPort   = CELL_SPURS_EVENT_FLAG_INVALID_SPU_PORT;

	if (taskset)
	{
		eventFlag->addr = taskset.addr();
	}
	else
	{
		eventFlag->isIwl = 1;
		eventFlag->addr  = spurs.addr();
	}

	return CELL_OK;
}

/// Reset a SPURS event flag
s32 cellSpursEventFlagClear(vm::ptr<CellSpursEventFlag> eventFlag, u16 bits)
{
	cellSpurs.Warning("cellSpursEventFlagClear(eventFlag=*0x%x, bits=0x%x)", eventFlag, bits);

	if (!eventFlag)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!eventFlag.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	eventFlag->events &= ~bits;
	return CELL_OK;
}

/// Set a SPURS event flag
s32 cellSpursEventFlagSet(PPUThread& ppu, vm::ptr<CellSpursEventFlag> eventFlag, u16 bits)
{
	cellSpurs.Warning("cellSpursEventFlagSet(eventFlag=*0x%x, bits=0x%x)", eventFlag, bits);

	if (!eventFlag)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!eventFlag.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (eventFlag->direction != CELL_SPURS_EVENT_FLAG_SPU2PPU && eventFlag->direction != CELL_SPURS_EVENT_FLAG_ANY2ANY)
	{
		return CELL_SPURS_TASK_ERROR_PERM;
	}

	bool send;
	u8   ppuWaitSlot;
	u16  ppuEvents;
	u16  pendingRecv;
	u16  pendingRecvTaskEvents[16];
	eventFlag->ctrl.atomic_op([eventFlag, bits, &send, &ppuWaitSlot, &ppuEvents, &pendingRecv, &pendingRecvTaskEvents](CellSpursEventFlag::ControlSyncVar& ctrl)
	{
		send        = false;
		ppuWaitSlot = 0;
		ppuEvents   = 0;
		pendingRecv = 0;

		u16 eventsToClear = 0;

		if (eventFlag->direction == CELL_SPURS_EVENT_FLAG_ANY2ANY && ctrl.ppuWaitMask)
		{
			u16 ppuRelevantEvents = (ctrl.events | bits) & ctrl.ppuWaitMask;

			// Unblock the waiting PPU thread if either all the bits being waited by the thread have been set or
			// if the wait mode of the thread is OR and atleast one bit the thread is waiting on has been set
			if ((ctrl.ppuWaitMask & ~ppuRelevantEvents) == 0 ||
				((ctrl.ppuWaitSlotAndMode & 0x0F) == CELL_SPURS_EVENT_FLAG_OR && ppuRelevantEvents != 0))
			{
				ctrl.ppuPendingRecv = 1;
				ctrl.ppuWaitMask    = 0;
				ppuEvents      = ppuRelevantEvents;
				eventsToClear  = ppuRelevantEvents;
				ppuWaitSlot    = ctrl.ppuWaitSlotAndMode >> 4;
				send           = true;
			}
		}

		s32 i                 = CELL_SPURS_EVENT_FLAG_MAX_WAIT_SLOTS - 1;
		s32 j                 = 0;
		u16 relevantWaitSlots = eventFlag->spuTaskUsedWaitSlots & ~ctrl.spuTaskPendingRecv;
		while (relevantWaitSlots)
		{
			if (relevantWaitSlots & 0x0001)
			{
				u16 spuTaskRelevantEvents = (ctrl.events | bits) & eventFlag->spuTaskWaitMask[i];

				// Unblock the waiting SPU task if either all the bits being waited by the task have been set or
				// if the wait mode of the task is OR and atleast one bit the thread is waiting on has been set
				if ((eventFlag->spuTaskWaitMask[i] & ~spuTaskRelevantEvents) == 0 || 
					(((eventFlag->spuTaskWaitMode >> j) & 0x0001) == CELL_SPURS_EVENT_FLAG_OR && spuTaskRelevantEvents != 0))
				{
					eventsToClear            |= spuTaskRelevantEvents;
					pendingRecv              |= 1 << j;
					pendingRecvTaskEvents[j]  = spuTaskRelevantEvents;
				}
			}

			relevantWaitSlots >>= 1;
			i--;
			j++;
		}

		ctrl.events             |= bits;
		ctrl.spuTaskPendingRecv |= pendingRecv;

		// If the clear flag is AUTO then clear the bits comnsumed by all tasks marked to be unblocked
		if (eventFlag->clearMode == CELL_SPURS_EVENT_FLAG_CLEAR_AUTO)
		{
			 ctrl.events &= ~eventsToClear;
		}

		//eventFlagControl = ((u64)events << 48) | ((u64)spuTaskPendingRecv << 32) | ((u64)ppuWaitMask << 16) | ((u64)ppuWaitSlotAndMode << 8) | (u64)ppuPendingRecv;
	});

	if (send)
	{
		// Signal the PPU thread to be woken up
		eventFlag->pendingRecvTaskEvents[ppuWaitSlot] = ppuEvents;
		if (s32 rc = sys_event_port_send(eventFlag->eventPortId, 0, 0, 0))
		{
			throw EXCEPTION("sys_event_port_send() failed (0x%x)", rc);
		}
	}

	if (pendingRecv)
	{
		// Signal each SPU task whose conditions have been met to be woken up
		for (s32 i = 0; i < CELL_SPURS_EVENT_FLAG_MAX_WAIT_SLOTS; i++)
		{
			if (pendingRecv & (0x8000 >> i))
			{
				eventFlag->pendingRecvTaskEvents[i] = pendingRecvTaskEvents[i];
				const vm::var<vm::ptr<CellSpursTaskset>> taskset(ppu);
				if (eventFlag->isIwl)
				{
					cellSpursLookUpTasksetAddress(ppu, vm::ptr<CellSpurs>::make((u32)eventFlag->addr), taskset, eventFlag->waitingTaskWklId[i]);
				}
				else
				{
					taskset->set((u32)eventFlag->addr);
				}

				auto rc = _cellSpursSendSignal(ppu, *taskset, eventFlag->waitingTaskId[i]);
				if (rc == CELL_SPURS_TASK_ERROR_INVAL || rc == CELL_SPURS_TASK_ERROR_STAT)
				{
					return CELL_SPURS_TASK_ERROR_FATAL;
				}

				if (rc != CELL_OK)
				{
					throw EXCEPTION("");
				}
			}
		}
	}

	return CELL_OK;
}

/// Wait for SPURS event flag
s32 spursEventFlagWait(PPUThread& ppu, vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u16> mask, u32 mode, u32 block)
{
	if (!eventFlag || !mask)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!eventFlag.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (mode > CELL_SPURS_EVENT_FLAG_WAIT_MODE_LAST)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	if (eventFlag->direction != CELL_SPURS_EVENT_FLAG_SPU2PPU && eventFlag->direction != CELL_SPURS_EVENT_FLAG_ANY2ANY)
	{
		return CELL_SPURS_TASK_ERROR_PERM;
	}

	if (block && eventFlag->spuPort == CELL_SPURS_EVENT_FLAG_INVALID_SPU_PORT)
	{
		return CELL_SPURS_TASK_ERROR_STAT;
	}

	if (eventFlag->ctrl.data.ppuWaitMask || eventFlag->ctrl.data.ppuPendingRecv)
	{
		return CELL_SPURS_TASK_ERROR_BUSY;
	}

	bool recv;
	s32  rc;
	u16  receivedEvents;
	eventFlag->ctrl.atomic_op([eventFlag, mask, mode, block, &recv, &rc, &receivedEvents](CellSpursEventFlag::ControlSyncVar& ctrl)
	{
		u16 relevantEvents = ctrl.events & *mask;
		if (eventFlag->direction == CELL_SPURS_EVENT_FLAG_ANY2ANY)
		{
			// Make sure the wait mask and mode specified does not conflict with that of the already waiting tasks.
			// Conflict scenarios:
			// OR  vs OR  - A conflict never occurs
			// OR  vs AND - A conflict occurs if the masks for the two tasks overlap
			// AND vs AND - A conflict occurs if the masks for the two tasks are not the same

			// Determine the set of all already waiting tasks whose wait mode/mask can possibly conflict with the specified wait mode/mask.
			// This set is equal to 'set of all tasks waiting' - 'set of all tasks whose wait conditions have been met'.
			// If the wait mode is OR, we prune the set of all tasks that are waiting in OR mode from the set since a conflict cannot occur
			// with an already waiting task in OR mode.
			u16 relevantWaitSlots = eventFlag->spuTaskUsedWaitSlots & ~ctrl.spuTaskPendingRecv;
			if (mode == CELL_SPURS_EVENT_FLAG_OR)
			{
				relevantWaitSlots &= eventFlag->spuTaskWaitMode;
			}

			s32 i = CELL_SPURS_EVENT_FLAG_MAX_WAIT_SLOTS - 1;
			while (relevantWaitSlots)
			{
				if (relevantWaitSlots & 0x0001)
				{
					if (eventFlag->spuTaskWaitMask[i] & *mask && eventFlag->spuTaskWaitMask[i] != *mask)
					{
						rc = CELL_SPURS_TASK_ERROR_AGAIN;
						return;
					}
				}

				relevantWaitSlots >>= 1;
				i--;
			}
		}

		// There is no need to block if all bits required by the wait operation have already been set or
		// if the wait mode is OR and atleast one of the bits required by the wait operation has been set.
		if ((*mask & ~relevantEvents) == 0 || (mode == CELL_SPURS_EVENT_FLAG_OR && relevantEvents))
		{
			// If the clear flag is AUTO then clear the bits comnsumed by this thread
			if (eventFlag->clearMode == CELL_SPURS_EVENT_FLAG_CLEAR_AUTO)
			{
				ctrl.events &= ~relevantEvents;
			}

			recv           = false;
			receivedEvents = relevantEvents;
		}
		else
		{
			// If we reach here it means that the conditions for this thread have not been met.
			// If this is a try wait operation then do not block but return an error code.
			if (block == 0)
			{
				rc = CELL_SPURS_TASK_ERROR_BUSY;
				return;
			}

			ctrl.ppuWaitSlotAndMode = 0;
			if (eventFlag->direction == CELL_SPURS_EVENT_FLAG_ANY2ANY)
			{
				// Find an unsed wait slot
				s32 i                    = 0;
				u16 spuTaskUsedWaitSlots = eventFlag->spuTaskUsedWaitSlots;
				while (spuTaskUsedWaitSlots & 0x0001)
				{
					spuTaskUsedWaitSlots >>= 1;
					i++;
				}

				if (i == CELL_SPURS_EVENT_FLAG_MAX_WAIT_SLOTS)
				{
					// Event flag has no empty wait slots
					rc = CELL_SPURS_TASK_ERROR_BUSY;
					return;
				}

				// Mark the found wait slot as used by this thread
				ctrl.ppuWaitSlotAndMode = (CELL_SPURS_EVENT_FLAG_MAX_WAIT_SLOTS - 1 - i) << 4;
			}

			// Save the wait mask and mode for this thread
			ctrl.ppuWaitSlotAndMode |= mode;
			ctrl.ppuWaitMask         = *mask;

			recv = true;
		}

		//eventFlagControl = ((u64)events << 48) | ((u64)spuTaskPendingRecv << 32) | ((u64)ppuWaitMask << 16) | ((u64)ppuWaitSlotAndMode << 8) | (u64)ppuPendingRecv;
		rc = CELL_OK;
	});

	if (rc != CELL_OK)
	{
		return rc;
	}

	if (recv)
	{
		// Block till something happens
		if (s32 rc = sys_event_queue_receive(ppu, eventFlag->eventQueueId, vm::null, 0))
		{
			throw EXCEPTION("sys_event_queue_receive() failed (0x%x)", rc);
		}

		s32 i = 0;
		if (eventFlag->direction == CELL_SPURS_EVENT_FLAG_ANY2ANY)
		{
			i = eventFlag->ctrl.data.ppuWaitSlotAndMode >> 4;
		}

		*mask = eventFlag->pendingRecvTaskEvents[i];
		eventFlag->ctrl.data.ppuPendingRecv = 0;
	}

	*mask = receivedEvents;
	return CELL_OK;
}

/// Wait for SPURS event flag
s32 cellSpursEventFlagWait(PPUThread& ppu, vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u16> mask, u32 mode)
{
	cellSpurs.Warning("cellSpursEventFlagWait(eventFlag=*0x%x, mask=*0x%x, mode=%d)", eventFlag, mask, mode);

	return spursEventFlagWait(ppu, eventFlag, mask, mode, 1 /*block*/);
}

/// Check SPURS event flag
s32 cellSpursEventFlagTryWait(PPUThread& ppu, vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u16> mask, u32 mode)
{
	cellSpurs.Warning("cellSpursEventFlagTryWait(eventFlag=*0x%x, mask=*0x%x, mode=0x%x)", eventFlag, mask, mode);

	return spursEventFlagWait(ppu, eventFlag, mask, mode, 0 /*block*/);
}

/// Attach an LV2 event queue to a SPURS event flag
s32 cellSpursEventFlagAttachLv2EventQueue(PPUThread& ppu, vm::ptr<CellSpursEventFlag> eventFlag)
{
	cellSpurs.Warning("cellSpursEventFlagAttachLv2EventQueue(eventFlag=*0x%x)", eventFlag);

	if (!eventFlag)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!eventFlag.aligned())
	{
		return CELL_SPURS_TASK_ERROR_AGAIN;
	}

	if (eventFlag->direction != CELL_SPURS_EVENT_FLAG_SPU2PPU && eventFlag->direction != CELL_SPURS_EVENT_FLAG_ANY2ANY)
	{
		return CELL_SPURS_TASK_ERROR_PERM;
	}

	if (eventFlag->spuPort != CELL_SPURS_EVENT_FLAG_INVALID_SPU_PORT)
	{
		return CELL_SPURS_TASK_ERROR_STAT;
	}

	vm::ptr<CellSpurs> spurs;
	if (eventFlag->isIwl == 1)
	{
		spurs.set((u32)eventFlag->addr);
	}
	else
	{
		auto taskset = vm::ptr<CellSpursTaskset>::make((u32)eventFlag->addr);
		spurs = taskset->spurs;
	}

	vm::var<u32>       eventQueueId(ppu);
	vm::var<u8>        port(ppu);
	vm::var<char>      evqName(ppu, 8);
	memcpy(evqName.get_ptr(), "_spuEvF", 8);

	auto failure = [](s32 rc) -> s32
	{
		// Return rc if its an error code from SPURS otherwise convert the error code to a SPURS task error code
		return (rc & 0x0FFF0000) == 0x00410000 ? rc : (0x80410900 | (rc & 0xFF));
	};

	if (s32 rc = spursCreateLv2EventQueue(ppu, spurs, eventQueueId, port, 1, evqName))
	{
		return failure(rc);
	}

	auto success = [&]
	{
		eventFlag->eventQueueId = *eventQueueId;
		eventFlag->spuPort      = *port;
	};

	if (eventFlag->direction == CELL_SPURS_EVENT_FLAG_ANY2ANY)
	{
		vm::var<u32> eventPortId(ppu);

		s32 rc = sys_event_port_create(eventPortId, SYS_EVENT_PORT_LOCAL, 0);
		if (rc == CELL_OK)
		{
			rc = sys_event_port_connect_local(*eventPortId, *eventQueueId);
			if (rc == CELL_OK)
			{
				eventFlag->eventPortId = *eventPortId;
				return success(), CELL_OK;
			}

			sys_event_port_destroy(*eventPortId);
		}

		if (spursDetachLv2EventQueue(spurs, *port, true /*spursCreated*/) == CELL_OK)
		{
			sys_event_queue_destroy(*eventQueueId, SYS_EVENT_QUEUE_DESTROY_FORCE);
		}

		return failure(rc);
	}

	return success(), CELL_OK;
}

/// Detach an LV2 event queue from SPURS event flag
s32 cellSpursEventFlagDetachLv2EventQueue(vm::ptr<CellSpursEventFlag> eventFlag)
{
	cellSpurs.Warning("cellSpursEventFlagDetachLv2EventQueue(eventFlag=*0x%x)", eventFlag);

	if (!eventFlag)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!eventFlag.aligned())
	{
		return CELL_SPURS_TASK_ERROR_AGAIN;
	}

	if (eventFlag->direction != CELL_SPURS_EVENT_FLAG_SPU2PPU && eventFlag->direction != CELL_SPURS_EVENT_FLAG_ANY2ANY)
	{
		return CELL_SPURS_TASK_ERROR_PERM;
	}

	if (eventFlag->spuPort == CELL_SPURS_EVENT_FLAG_INVALID_SPU_PORT)
	{
		return CELL_SPURS_TASK_ERROR_STAT;
	}

	if (eventFlag->ctrl.data.ppuWaitMask || eventFlag->ctrl.data.ppuPendingRecv)
	{
		return CELL_SPURS_TASK_ERROR_BUSY;
	}

	const u8 port = eventFlag->spuPort;

	eventFlag->spuPort = CELL_SPURS_EVENT_FLAG_INVALID_SPU_PORT;

	vm::ptr<CellSpurs> spurs;
	if (eventFlag->isIwl == 1)
	{
		spurs.set((u32)eventFlag->addr);
	}
	else
	{
		auto taskset = vm::ptr<CellSpursTaskset>::make((u32)eventFlag->addr);
		spurs = taskset->spurs;
	}

	if (eventFlag->direction == CELL_SPURS_EVENT_FLAG_ANY2ANY)
	{
		sys_event_port_disconnect(eventFlag->eventPortId);
		sys_event_port_destroy(eventFlag->eventPortId);
	}

	auto rc = spursDetachLv2EventQueue(spurs, port, true /*spursCreated*/);

	if (rc == CELL_OK)
	{
		rc = sys_event_queue_destroy(eventFlag->eventQueueId, SYS_EVENT_QUEUE_DESTROY_FORCE);
	}

	return CELL_OK;
}

/// Get send-receive direction of the SPURS event flag
s32 cellSpursEventFlagGetDirection(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u32> direction)
{
	cellSpurs.Warning("cellSpursEventFlagGetDirection(eventFlag=*0x%x, direction=*0x%x)", eventFlag, direction);

	if (!eventFlag || !direction)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!eventFlag.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	*direction = eventFlag->direction;
	return CELL_OK;
}

/// Get clearing mode of SPURS event flag
s32 cellSpursEventFlagGetClearMode(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u32> clear_mode)
{
	cellSpurs.Warning("cellSpursEventFlagGetClearMode(eventFlag=*0x%x, clear_mode=*0x%x)", eventFlag, clear_mode);

	if (!eventFlag || !clear_mode)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!eventFlag.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	*clear_mode = eventFlag->clearMode;
	return CELL_OK;
}

/// Get address of taskset to which the SPURS event flag belongs
s32 cellSpursEventFlagGetTasksetAddress(vm::ptr<CellSpursEventFlag> eventFlag, vm::pptr<CellSpursTaskset> taskset)
{
	cellSpurs.Warning("cellSpursEventFlagGetTasksetAddress(eventFlag=*0x%x, taskset=**0x%x)", eventFlag, taskset);

	if (!eventFlag || !taskset)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!eventFlag.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	taskset->set(eventFlag->isIwl ? 0u : VM_CAST(eventFlag->addr));
	return CELL_OK;
}

s32 _cellSpursLFQueueInitialize(vm::ptr<void> pTasksetOrSpurs, vm::ptr<CellSpursLFQueue> pQueue, vm::cptr<void> buffer, u32 size, u32 depth, u32 direction)
{
	cellSpurs.Todo("_cellSpursLFQueueInitialize(pTasksetOrSpurs=*0x%x, pQueue=*0x%x, buffer=*0x%x, size=0x%x, depth=0x%x, direction=%d)", pTasksetOrSpurs, pQueue, buffer, size, depth, direction);

	return SyncErrorToSpursError(cellSyncLFQueueInitialize(pQueue, buffer, size, depth, direction, pTasksetOrSpurs));
}

s32 _cellSpursLFQueuePushBody()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursLFQueueAttachLv2EventQueue(vm::ptr<CellSyncLFQueue> queue)
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursLFQueueDetachLv2EventQueue(vm::ptr<CellSyncLFQueue> queue)
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 _cellSpursLFQueuePopBody()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursLFQueueGetTasksetAddress()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 _cellSpursQueueInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursQueuePopBody()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursQueuePushBody()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursQueueAttachLv2EventQueue()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursQueueDetachLv2EventQueue()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursQueueGetTasksetAddress()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursQueueClear()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursQueueDepth()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursQueueGetEntrySize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursQueueSize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursQueueGetDirection()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 spursCreateTaskset(PPUThread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, u64 args, vm::cptr<u8[8]> priority, u32 max_contention, vm::cptr<char> name, u32 size, s32 enable_clear_ls)
{
	if (!spurs || !taskset)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned() || !taskset.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	memset(taskset.get_ptr(), 0, size);

	taskset->spurs = spurs;
	taskset->args = args;
	taskset->enable_clear_ls = enable_clear_ls > 0 ? 1 : 0;
	taskset->size = size;

	const vm::var<CellSpursWorkloadAttribute> wkl_attr(ppu);
	_cellSpursWorkloadAttributeInitialize(wkl_attr, 1 /*revision*/, 0x33 /*sdk_version*/, vm::cptr<void>::make(SPURS_IMG_ADDR_TASKSET_PM), 0x1E40 /*pm_size*/,
		taskset.addr(), priority, 8 /*min_contention*/, max_contention);
	// TODO: Check return code

	cellSpursWorkloadAttributeSetName(wkl_attr, vm::null, name);
	// TODO: Check return code

	// TODO: cellSpursWorkloadAttributeSetShutdownCompletionEventHook(wkl_attr, hook, taskset);
	// TODO: Check return code

	vm::var<u32> wid(ppu);
	cellSpursAddWorkloadWithAttribute(spurs, wid, wkl_attr);
	// TODO: Check return code

	taskset->wkl_flag_wait_task = 0x80;
	taskset->wid                = *wid;
	// TODO: cellSpursSetExceptionEventHandler(spurs, wid, hook, taskset);
	// TODO: Check return code

	return CELL_OK;
}

s32 cellSpursCreateTasksetWithAttribute(PPUThread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursTasksetAttribute> attr)
{
	cellSpurs.Warning("cellSpursCreateTasksetWithAttribute(spurs=*0x%x, taskset=*0x%x, attr=*0x%x)", spurs, taskset, attr);

	if (!attr)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (attr->revision != CELL_SPURS_TASKSET_ATTRIBUTE_REVISION)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	auto rc = spursCreateTaskset(ppu, spurs, taskset, attr->args, attr.ptr(&CellSpursTasksetAttribute::priority), attr->max_contention, attr->name, attr->taskset_size, attr->enable_clear_ls);

	if (attr->taskset_size >= sizeof32(CellSpursTaskset2))
	{
		// TODO: Implement this
	}

	return rc;
}

s32 cellSpursCreateTaskset(PPUThread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, u64 args, vm::cptr<u8[8]> priority, u32 maxContention)
{
	cellSpurs.Warning("cellSpursCreateTaskset(spurs=*0x%x, taskset=*0x%x, args=0x%llx, priority=*0x%x, maxContention=%d)", spurs, taskset, args, priority, maxContention);

	return spursCreateTaskset(ppu, spurs, taskset, args, priority, maxContention, vm::null, sizeof32(CellSpursTaskset), 0);
}

s32 cellSpursJoinTaskset(vm::ptr<CellSpursTaskset> taskset)
{
	cellSpurs.Warning("cellSpursJoinTaskset(taskset=*0x%x)", taskset);

	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursGetTasksetId(vm::ptr<CellSpursTaskset> taskset, vm::ptr<u32> wid)
{
	cellSpurs.Warning("cellSpursGetTasksetId(taskset=*0x%x, wid=*0x%x)", taskset, wid);

	if (!taskset || !wid)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!taskset.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (taskset->wid >= CELL_SPURS_MAX_WORKLOAD)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	*wid = taskset->wid;
	return CELL_OK;
}

s32 cellSpursShutdownTaskset(vm::ptr<CellSpursTaskset> taskset)
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 spursCreateTask(vm::ptr<CellSpursTaskset> taskset, vm::ptr<u32> task_id, vm::cptr<void> elf, vm::cptr<void> context, u32 size, vm::ptr<CellSpursTaskLsPattern> ls_pattern, vm::ptr<CellSpursTaskArgument> arg)
{
	if (!taskset || !elf)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (elf % 16)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (spursGetSdkVersion() < 0x27FFFF)
	{
		if (context % 16)
		{
			return CELL_SPURS_TASK_ERROR_ALIGN;
		}
	}
	else
	{
		if (context % 128)
		{
			return CELL_SPURS_TASK_ERROR_ALIGN;
		}
	}

	u32 alloc_ls_blocks = 0;

	if (context)
	{
		if (size < CELL_SPURS_TASK_EXECUTION_CONTEXT_SIZE)
		{
			return CELL_SPURS_TASK_ERROR_INVAL;
		}

		alloc_ls_blocks = size > 0x3D400 ? 0x7A : ((size - 0x400) >> 11);
		if (ls_pattern)
		{
			v128 ls_pattern_128 = v128::from64r(ls_pattern->_u64[0], ls_pattern->_u64[1]);
			u32 ls_blocks       = 0;
			for (auto i = 0; i < 128; i++)
			{
				if (ls_pattern_128._bit[i])
				{
					ls_blocks++;
				}
			}

			if (ls_blocks > alloc_ls_blocks)
			{
				return CELL_SPURS_TASK_ERROR_INVAL;
			}

			v128 _0 = v128::from32(0);
			if ((ls_pattern_128 & v128::from32r(0xFC000000)) != _0)
			{
				// Prevent save/restore to SPURS management area
				return CELL_SPURS_TASK_ERROR_INVAL;
			}
		}
	}
	else
	{
		alloc_ls_blocks = 0;
	}

	// TODO: Verify the ELF header is proper and all its load segments are at address >= 0x3000

	u32 tmp_task_id;
	for (tmp_task_id = 0; tmp_task_id < CELL_SPURS_MAX_TASK; tmp_task_id++)
	{
		if (!taskset->enabled.value()._bit[tmp_task_id])
		{
			auto enabled              = taskset->enabled.value();
			enabled._bit[tmp_task_id] = true;
			taskset->enabled        = enabled;
			break;
		}
	}

	if (tmp_task_id >= CELL_SPURS_MAX_TASK)
	{
		return CELL_SPURS_TASK_ERROR_AGAIN;
	}

	taskset->task_info[tmp_task_id].elf = elf;
	taskset->task_info[tmp_task_id].context_save_storage_and_alloc_ls_blocks = (context.addr() | alloc_ls_blocks);
	taskset->task_info[tmp_task_id].args                                     = *arg;
	if (ls_pattern)
	{
		taskset->task_info[tmp_task_id].ls_pattern = *ls_pattern;
	}

	*task_id = tmp_task_id;
	return CELL_OK;
}

s32 spursTaskStart(PPUThread& ppu, vm::ptr<CellSpursTaskset> taskset, u32 taskId)
{
	auto pendingReady         = taskset->pending_ready.value();
	pendingReady._bit[taskId] = true;
	taskset->pending_ready  = pendingReady;

	cellSpursSendWorkloadSignal(taskset->spurs, taskset->wid);

	if (s32 rc = cellSpursWakeUp(ppu, taskset->spurs))
	{
		if (rc == CELL_SPURS_POLICY_MODULE_ERROR_STAT)
		{
			rc = CELL_SPURS_TASK_ERROR_STAT;
		}
		else
		{
			throw EXCEPTION("cellSpursWakeUp() failed (0x%x)", rc);
		}
	}

	return CELL_OK;
}

s32 cellSpursCreateTask(PPUThread& ppu, vm::ptr<CellSpursTaskset> taskset, vm::ptr<u32> taskId, vm::cptr<void> elf, vm::cptr<void> context, u32 size, vm::ptr<CellSpursTaskLsPattern> lsPattern, vm::ptr<CellSpursTaskArgument> argument)
{
	cellSpurs.Warning("cellSpursCreateTask(taskset=*0x%x, taskID=*0x%x, elf=*0x%x, context=*0x%x, size=0x%x, lsPattern=*0x%x, argument=*0x%x)", taskset, taskId, elf, context, size, lsPattern, argument);

	if (!taskset)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!taskset.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	auto rc = spursCreateTask(taskset, taskId, elf, context, size, lsPattern, argument);
	if (rc != CELL_OK) 
	{
		return rc;
	}

	rc = spursTaskStart(ppu, taskset, *taskId);
	if (rc != CELL_OK) 
	{
		return rc;
	}

	return CELL_OK;
}

s32 _cellSpursSendSignal(PPUThread& ppu, vm::ptr<CellSpursTaskset> taskset, u32 taskId)
{
	if (!taskset)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!taskset.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (taskId >= CELL_SPURS_MAX_TASK || taskset->wid >= CELL_SPURS_MAX_WORKLOAD2)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

    be_t<v128> _0(v128::from32(0));
	bool disabled = taskset->enabled.value()._bit[taskId];
	auto invalid  = (taskset->ready & taskset->pending_ready) != _0 || (taskset->running & taskset->waiting) != _0 || disabled ||
					((taskset->running | taskset->ready | taskset->pending_ready | taskset->waiting | taskset->signalled) & ~taskset->enabled) != _0;

	if (invalid)
	{
		return CELL_SPURS_TASK_ERROR_SRCH;
	}

	auto shouldSignal      = ((taskset->waiting & ~taskset->signalled) & be_t<v128>(v128::fromBit(taskId))) != _0 ? true : false;
	auto signalled         = taskset->signalled.value();
	signalled._bit[taskId] = true;
	taskset->signalled   = signalled;
	if (shouldSignal)
	{
		cellSpursSendWorkloadSignal(taskset->spurs, taskset->wid);
		auto rc = cellSpursWakeUp(ppu, taskset->spurs);
		if (rc == CELL_SPURS_POLICY_MODULE_ERROR_STAT)
		{
			return CELL_SPURS_TASK_ERROR_STAT;
		}

		if (rc != CELL_OK)
		{
			assert(0);
		}
	}

	return CELL_OK;
}

s32 cellSpursCreateTaskWithAttribute()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursTasksetAttributeSetName(vm::ptr<CellSpursTasksetAttribute> attr, vm::cptr<char> name)
{
	cellSpurs.Warning("cellSpursTasksetAttributeSetName(attr=*0x%x, name=*0x%x)", attr, name);

	if (!attr || !name)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	attr->name = name;
	return CELL_OK;
}

s32 cellSpursTasksetAttributeSetTasksetSize(vm::ptr<CellSpursTasksetAttribute> attr, u32 size)
{
	cellSpurs.Warning("cellSpursTasksetAttributeSetTasksetSize(attr=*0x%x, size=0x%x)", attr, size);

	if (!attr)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (size != sizeof32(CellSpursTaskset) && size != sizeof32(CellSpursTaskset2))
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	attr->taskset_size = size;
	return CELL_OK;
}

s32 cellSpursTasksetAttributeEnableClearLS(vm::ptr<CellSpursTasksetAttribute> attr, s32 enable)
{
	cellSpurs.Warning("cellSpursTasksetAttributeEnableClearLS(attr=*0x%x, enable=%d)", attr, enable);

	if (!attr)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	attr->enable_clear_ls = enable ? 1 : 0;
	return CELL_OK;
}

s32 _cellSpursTasksetAttribute2Initialize(vm::ptr<CellSpursTasksetAttribute2> attribute, u32 revision)
{
	cellSpurs.Warning("_cellSpursTasksetAttribute2Initialize(attribute=*0x%x, revision=%d)", attribute, revision);

	memset(attribute.get_ptr(), 0, sizeof(CellSpursTasksetAttribute2));
	attribute->revision = revision;
	attribute->name = vm::null;
	attribute->args = 0;

	for (s32 i = 0; i < 8; i++)
	{
		attribute->priority[i] = 1;
	}

	attribute->max_contention = 8;
	attribute->enable_clear_ls = 0;
	attribute->task_name_buffer.set(0);
	return CELL_OK;
}

s32 cellSpursTaskExitCodeGet()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursTaskExitCodeInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursTaskExitCodeTryGet()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursTaskGetLoadableSegmentPattern()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursTaskGetReadOnlyAreaPattern()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursTaskGenerateLsPattern()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 _cellSpursTaskAttributeInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursTaskAttributeSetExitCodeContainer()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 _cellSpursTaskAttribute2Initialize(vm::ptr<CellSpursTaskAttribute2> attribute, u32 revision)
{
	cellSpurs.Warning("_cellSpursTaskAttribute2Initialize(attribute=*0x%x, revision=%d)", attribute, revision);

	attribute->revision = revision;
	attribute->sizeContext = 0;
	attribute->eaContext = 0;

	for (s32 c = 0; c < 4; c++)
	{
		attribute->lsPattern._u32[c] = 0;
	}

	attribute->name = vm::null;

	return CELL_OK;
}

s32 cellSpursTaskGetContextSaveAreaSize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursCreateTaskset2(PPUThread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursTasksetAttribute2> attr)
{
	cellSpurs.Warning("cellSpursCreateTaskset2(spurs=*0x%x, taskset=*0x%x, attr=*0x%x)", spurs, taskset, attr);

	const vm::var<CellSpursTasksetAttribute2> tmp_attr(ppu);

	if (!attr)
	{
		attr = tmp_attr;
		_cellSpursTasksetAttribute2Initialize(attr, 0);
	}

	if (s32 rc = spursCreateTaskset(ppu, spurs, taskset, attr->args, attr.ptr(&CellSpursTasksetAttribute2::priority), attr->max_contention, attr->name, sizeof32(CellSpursTaskset2), attr->enable_clear_ls))
	{
		return rc;
	}

	if (!attr->task_name_buffer.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	// TODO: Implement rest of the function
	return CELL_OK;
}

s32 cellSpursCreateTask2()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJoinTask2()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursTryJoinTask2()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursDestroyTaskset2()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursCreateTask2WithBinInfo()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursTasksetSetExceptionEventHandler(vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursTasksetExceptionEventHandler> handler, vm::ptr<u64> arg)
{
	cellSpurs.Warning("cellSpursTasksetSetExceptionEventHandler(taskset=*0x%x, handler=*0x%x, arg=*0x%x)", taskset, handler, arg);

	if (!taskset || !handler)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!taskset.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (taskset->wid >= CELL_SPURS_MAX_WORKLOAD)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	if (taskset->exception_handler)
	{
		return CELL_SPURS_TASK_ERROR_BUSY;
	}

	taskset->exception_handler = handler;
	taskset->exception_handler_arg = arg;
	return CELL_OK;
}

s32 cellSpursTasksetUnsetExceptionEventHandler(vm::ptr<CellSpursTaskset> taskset)
{
	cellSpurs.Warning("cellSpursTasksetUnsetExceptionEventHandler(taskset=*0x%x)", taskset);

	if (!taskset)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!taskset.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (taskset->wid >= CELL_SPURS_MAX_WORKLOAD)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	taskset->exception_handler.set(0);
	taskset->exception_handler_arg.set(0);
	return CELL_OK;
}

s32 cellSpursLookUpTasksetAddress(PPUThread& ppu, vm::ptr<CellSpurs> spurs, vm::pptr<CellSpursTaskset> taskset, u32 id)
{
	cellSpurs.Warning("cellSpursLookUpTasksetAddress(spurs=*0x%x, taskset=**0x%x, id=0x%x)", spurs, taskset, id);

	if (!taskset)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	vm::var<u64> data(ppu);
	if (s32 rc = cellSpursGetWorkloadData(spurs, data, id))
	{
		// Convert policy module error code to a task error code
		return rc ^ 0x100;
	}

	taskset->set((u32)*data);
	return CELL_OK;
}

s32 cellSpursTasksetGetSpursAddress(vm::cptr<CellSpursTaskset> taskset, vm::ptr<u32> spurs)
{
	cellSpurs.Warning("cellSpursTasksetGetSpursAddress(taskset=*0x%x, spurs=**0x%x)", taskset, spurs);

	if (!taskset || !spurs)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!taskset.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (taskset->wid >= CELL_SPURS_MAX_WORKLOAD)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	*spurs = (u32)taskset->spurs.addr();
	return CELL_OK;
}

s32 cellSpursGetTasksetInfo()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 _cellSpursTasksetAttributeInitialize(vm::ptr<CellSpursTasksetAttribute> attribute, u32 revision, u32 sdk_version, u64 args, vm::cptr<u8> priority, u32 max_contention)
{
	cellSpurs.Warning("_cellSpursTasksetAttributeInitialize(attribute=*0x%x, revision=%d, skd_version=0x%x, args=0x%llx, priority=*0x%x, max_contention=%d)",
		attribute, revision, sdk_version, args, priority, max_contention);

	if (!attribute)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!attribute.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	for (u32 i = 0; i < 8; i++)
	{
		if (priority[i] > 0xF)
		{
			return CELL_SPURS_TASK_ERROR_INVAL;
		}
	}

	memset(attribute.get_ptr(), 0, sizeof(CellSpursTasksetAttribute));
	attribute->revision = revision;
	attribute->sdk_version = sdk_version;
	attribute->args = args;
	memcpy(attribute->priority, priority.get_ptr(), 8);
	attribute->taskset_size = 6400/*CellSpursTaskset::size*/;
	attribute->max_contention = max_contention;
	return CELL_OK;
}

s32 cellSpursCreateJobChainWithAttribute()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursCreateJobChain()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJoinJobChain()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursKickJobChain()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 _cellSpursJobChainAttributeInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursGetJobChainId()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobChainSetExceptionEventHandler()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobChainUnsetExceptionEventHandler()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursGetJobChainInfo()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobChainGetSpursAddress()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobGuardInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobChainAttributeSetName()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursShutdownJobChain()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobChainAttributeSetHaltOnError()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobChainAttributeSetJobTypeMemoryCheck()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobGuardNotify()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobGuardReset()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursRunJobChain()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobChainGetError()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursGetJobPipelineInfo()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobSetMaxGrab()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobHeaderSetJobbin2Param()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursAddUrgentCommand()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursAddUrgentCall()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursBarrierInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursBarrierGetTasksetAddress()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 _cellSpursSemaphoreInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursSemaphoreGetTasksetAddress()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

Module cellSpurs("cellSpurs", []()
{
	// Core 
	REG_FUNC(cellSpurs, cellSpursInitialize);
	REG_FUNC(cellSpurs, cellSpursInitializeWithAttribute);
	REG_FUNC(cellSpurs, cellSpursInitializeWithAttribute2);
	REG_FUNC(cellSpurs, cellSpursFinalize);
	REG_FUNC(cellSpurs, _cellSpursAttributeInitialize);
	REG_FUNC(cellSpurs, cellSpursAttributeSetMemoryContainerForSpuThread);
	REG_FUNC(cellSpurs, cellSpursAttributeSetNamePrefix);
	REG_FUNC(cellSpurs, cellSpursAttributeEnableSpuPrintfIfAvailable);
	REG_FUNC(cellSpurs, cellSpursAttributeSetSpuThreadGroupType);
	REG_FUNC(cellSpurs, cellSpursAttributeEnableSystemWorkload);
	REG_FUNC(cellSpurs, cellSpursGetSpuThreadGroupId);
	REG_FUNC(cellSpurs, cellSpursGetNumSpuThread);
	REG_FUNC(cellSpurs, cellSpursGetSpuThreadId);
	REG_FUNC(cellSpurs, cellSpursGetInfo);
	REG_FUNC(cellSpurs, cellSpursSetMaxContention);
	REG_FUNC(cellSpurs, cellSpursSetPriorities);
	REG_FUNC(cellSpurs, cellSpursSetPriority);
	REG_FUNC(cellSpurs, cellSpursSetPreemptionVictimHints);
	REG_FUNC(cellSpurs, cellSpursAttachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursDetachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursEnableExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursSetGlobalExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursUnsetGlobalExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursSetExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursUnsetExceptionEventHandler);

	// Event flag
	REG_FUNC(cellSpurs, _cellSpursEventFlagInitialize);
	REG_FUNC(cellSpurs, cellSpursEventFlagAttachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursEventFlagDetachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursEventFlagWait);
	REG_FUNC(cellSpurs, cellSpursEventFlagClear);
	REG_FUNC(cellSpurs, cellSpursEventFlagSet);
	REG_FUNC(cellSpurs, cellSpursEventFlagTryWait);
	REG_FUNC(cellSpurs, cellSpursEventFlagGetDirection);
	REG_FUNC(cellSpurs, cellSpursEventFlagGetClearMode);
	REG_FUNC(cellSpurs, cellSpursEventFlagGetTasksetAddress);

	// Taskset
	REG_FUNC(cellSpurs, cellSpursCreateTaskset);
	REG_FUNC(cellSpurs, cellSpursCreateTasksetWithAttribute);
	REG_FUNC(cellSpurs, _cellSpursTasksetAttributeInitialize);
	REG_FUNC(cellSpurs, _cellSpursTasksetAttribute2Initialize);
	REG_FUNC(cellSpurs, cellSpursTasksetAttributeSetName);
	REG_FUNC(cellSpurs, cellSpursTasksetAttributeSetTasksetSize);
	REG_FUNC(cellSpurs, cellSpursTasksetAttributeEnableClearLS);
	REG_FUNC(cellSpurs, cellSpursJoinTaskset);
	REG_FUNC(cellSpurs, cellSpursGetTasksetId);
	REG_FUNC(cellSpurs, cellSpursShutdownTaskset);
	REG_FUNC(cellSpurs, cellSpursCreateTask);
	REG_FUNC(cellSpurs, cellSpursCreateTaskWithAttribute);
	REG_FUNC(cellSpurs, _cellSpursTaskAttributeInitialize);
	REG_FUNC(cellSpurs, _cellSpursTaskAttribute2Initialize);
	REG_FUNC(cellSpurs, cellSpursTaskAttributeSetExitCodeContainer);
	REG_FUNC(cellSpurs, cellSpursTaskExitCodeGet);
	REG_FUNC(cellSpurs, cellSpursTaskExitCodeInitialize);
	REG_FUNC(cellSpurs, cellSpursTaskExitCodeTryGet);
	REG_FUNC(cellSpurs, cellSpursTaskGetLoadableSegmentPattern);
	REG_FUNC(cellSpurs, cellSpursTaskGetReadOnlyAreaPattern);
	REG_FUNC(cellSpurs, cellSpursTaskGenerateLsPattern);
	REG_FUNC(cellSpurs, cellSpursTaskGetContextSaveAreaSize);
	REG_FUNC(cellSpurs, _cellSpursSendSignal);
	REG_FUNC(cellSpurs, cellSpursCreateTaskset2);
	REG_FUNC(cellSpurs, cellSpursCreateTask2);
	REG_FUNC(cellSpurs, cellSpursJoinTask2);
	REG_FUNC(cellSpurs, cellSpursTryJoinTask2);
	REG_FUNC(cellSpurs, cellSpursDestroyTaskset2);
	REG_FUNC(cellSpurs, cellSpursCreateTask2WithBinInfo);
	REG_FUNC(cellSpurs, cellSpursLookUpTasksetAddress);
	REG_FUNC(cellSpurs, cellSpursTasksetGetSpursAddress);
	REG_FUNC(cellSpurs, cellSpursGetTasksetInfo);
	REG_FUNC(cellSpurs, cellSpursTasksetSetExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursTasksetUnsetExceptionEventHandler);

	// Job Chain
	REG_FUNC(cellSpurs, cellSpursCreateJobChain);
	REG_FUNC(cellSpurs, cellSpursCreateJobChainWithAttribute);
	REG_FUNC(cellSpurs, cellSpursShutdownJobChain);
	REG_FUNC(cellSpurs, cellSpursJoinJobChain);
	REG_FUNC(cellSpurs, cellSpursKickJobChain);
	REG_FUNC(cellSpurs, cellSpursRunJobChain);
	REG_FUNC(cellSpurs, cellSpursJobChainGetError);
	REG_FUNC(cellSpurs, _cellSpursJobChainAttributeInitialize);
	REG_FUNC(cellSpurs, cellSpursJobChainAttributeSetName);
	REG_FUNC(cellSpurs, cellSpursJobChainAttributeSetHaltOnError);
	REG_FUNC(cellSpurs, cellSpursJobChainAttributeSetJobTypeMemoryCheck);
	REG_FUNC(cellSpurs, cellSpursGetJobChainId);
	REG_FUNC(cellSpurs, cellSpursJobChainSetExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursJobChainUnsetExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursGetJobChainInfo);
	REG_FUNC(cellSpurs, cellSpursJobChainGetSpursAddress);

	// Job Guard
	REG_FUNC(cellSpurs, cellSpursJobGuardInitialize);
	REG_FUNC(cellSpurs, cellSpursJobGuardNotify);
	REG_FUNC(cellSpurs, cellSpursJobGuardReset);
	
	// LFQueue
	REG_FUNC(cellSpurs, _cellSpursLFQueueInitialize);
	REG_FUNC(cellSpurs, _cellSpursLFQueuePushBody);
	REG_FUNC(cellSpurs, cellSpursLFQueueAttachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursLFQueueDetachLv2EventQueue);
	REG_FUNC(cellSpurs, _cellSpursLFQueuePopBody);
	REG_FUNC(cellSpurs, cellSpursLFQueueGetTasksetAddress);

	// Queue
	REG_FUNC(cellSpurs, _cellSpursQueueInitialize);
	REG_FUNC(cellSpurs, cellSpursQueuePopBody);
	REG_FUNC(cellSpurs, cellSpursQueuePushBody);
	REG_FUNC(cellSpurs, cellSpursQueueAttachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursQueueDetachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursQueueGetTasksetAddress);
	REG_FUNC(cellSpurs, cellSpursQueueClear);
	REG_FUNC(cellSpurs, cellSpursQueueDepth);
	REG_FUNC(cellSpurs, cellSpursQueueGetEntrySize);
	REG_FUNC(cellSpurs, cellSpursQueueSize);
	REG_FUNC(cellSpurs, cellSpursQueueGetDirection);

	// Workload
	REG_FUNC(cellSpurs, cellSpursWorkloadAttributeSetName);
	REG_FUNC(cellSpurs, cellSpursWorkloadAttributeSetShutdownCompletionEventHook);
	REG_FUNC(cellSpurs, cellSpursAddWorkloadWithAttribute);
	REG_FUNC(cellSpurs, cellSpursAddWorkload);
	REG_FUNC(cellSpurs, cellSpursShutdownWorkload);
	REG_FUNC(cellSpurs, cellSpursWaitForWorkloadShutdown);
	REG_FUNC(cellSpurs, cellSpursRemoveWorkload);
	REG_FUNC(cellSpurs, cellSpursReadyCountStore);
	REG_FUNC(cellSpurs, cellSpursGetWorkloadFlag);
	REG_FUNC(cellSpurs, _cellSpursWorkloadFlagReceiver);
	REG_FUNC(cellSpurs, _cellSpursWorkloadAttributeInitialize);
	REG_FUNC(cellSpurs, cellSpursSendWorkloadSignal);
	REG_FUNC(cellSpurs, cellSpursGetWorkloadData);
	REG_FUNC(cellSpurs, cellSpursReadyCountAdd);
	REG_FUNC(cellSpurs, cellSpursReadyCountCompareAndSwap);
	REG_FUNC(cellSpurs, cellSpursReadyCountSwap);
	REG_FUNC(cellSpurs, cellSpursRequestIdleSpu);
	REG_FUNC(cellSpurs, cellSpursGetWorkloadInfo);
	REG_FUNC(cellSpurs, cellSpursGetSpuGuid);
	REG_FUNC(cellSpurs, _cellSpursWorkloadFlagReceiver2);
	REG_FUNC(cellSpurs, cellSpursGetJobPipelineInfo);
	REG_FUNC(cellSpurs, cellSpursJobSetMaxGrab);
	REG_FUNC(cellSpurs, cellSpursJobHeaderSetJobbin2Param);

	REG_FUNC(cellSpurs, cellSpursWakeUp);
	REG_FUNC(cellSpurs, cellSpursAddUrgentCommand);
	REG_FUNC(cellSpurs, cellSpursAddUrgentCall);

	REG_FUNC(cellSpurs, cellSpursBarrierInitialize);
	REG_FUNC(cellSpurs, cellSpursBarrierGetTasksetAddress);

	REG_FUNC(cellSpurs, _cellSpursSemaphoreInitialize);
	REG_FUNC(cellSpurs, cellSpursSemaphoreGetTasksetAddress);

	// Trace
	REG_FUNC(cellSpurs, cellSpursTraceInitialize);
	REG_FUNC(cellSpurs, cellSpursTraceStart);
	REG_FUNC(cellSpurs, cellSpursTraceStop);
	REG_FUNC(cellSpurs, cellSpursTraceFinalize);
});
