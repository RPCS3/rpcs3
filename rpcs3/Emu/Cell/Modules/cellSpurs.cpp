#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/Memory/vm_reservation.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/lv2/sys_lwmutex.h"
#include "Emu/Cell/lv2/sys_lwcond.h"
#include "Emu/Cell/lv2/sys_spu.h"
#include "Emu/Cell/lv2/sys_ppu_thread.h"
#include "Emu/Cell/lv2/sys_memory.h"
#include "Emu/Cell/lv2/sys_process.h"
#include "Emu/Cell/lv2/sys_semaphore.h"
#include "Emu/Cell/lv2/sys_event.h"
#include "sysPrxForUser.h"
#include "cellSpurs.h"

#include "util/asm.hpp"
#include "util/v128.hpp"
#include "util/simd.hpp"

LOG_CHANNEL(cellSpurs);

template <>
void fmt_class_string<CellSpursCoreError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(CELL_SPURS_CORE_ERROR_AGAIN);
		STR_CASE(CELL_SPURS_CORE_ERROR_INVAL);
		STR_CASE(CELL_SPURS_CORE_ERROR_NOMEM);
		STR_CASE(CELL_SPURS_CORE_ERROR_SRCH);
		STR_CASE(CELL_SPURS_CORE_ERROR_PERM);
		STR_CASE(CELL_SPURS_CORE_ERROR_BUSY);
		STR_CASE(CELL_SPURS_CORE_ERROR_STAT);
		STR_CASE(CELL_SPURS_CORE_ERROR_ALIGN);
		STR_CASE(CELL_SPURS_CORE_ERROR_NULL_POINTER);
		}

		return unknown;
	});
}

template <>
void fmt_class_string<CellSpursPolicyModuleError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(CELL_SPURS_POLICY_MODULE_ERROR_AGAIN);
		STR_CASE(CELL_SPURS_POLICY_MODULE_ERROR_INVAL);
		STR_CASE(CELL_SPURS_POLICY_MODULE_ERROR_NOSYS);
		STR_CASE(CELL_SPURS_POLICY_MODULE_ERROR_NOMEM);
		STR_CASE(CELL_SPURS_POLICY_MODULE_ERROR_SRCH);
		STR_CASE(CELL_SPURS_POLICY_MODULE_ERROR_NOENT);
		STR_CASE(CELL_SPURS_POLICY_MODULE_ERROR_NOEXEC);
		STR_CASE(CELL_SPURS_POLICY_MODULE_ERROR_DEADLK);
		STR_CASE(CELL_SPURS_POLICY_MODULE_ERROR_PERM);
		STR_CASE(CELL_SPURS_POLICY_MODULE_ERROR_BUSY);
		STR_CASE(CELL_SPURS_POLICY_MODULE_ERROR_ABORT);
		STR_CASE(CELL_SPURS_POLICY_MODULE_ERROR_FAULT);
		STR_CASE(CELL_SPURS_POLICY_MODULE_ERROR_CHILD);
		STR_CASE(CELL_SPURS_POLICY_MODULE_ERROR_STAT);
		STR_CASE(CELL_SPURS_POLICY_MODULE_ERROR_ALIGN);
		STR_CASE(CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER);
		}

		return unknown;
	});
}

template <>
void fmt_class_string<CellSpursTaskError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(CELL_SPURS_TASK_ERROR_AGAIN);
		STR_CASE(CELL_SPURS_TASK_ERROR_INVAL);
		STR_CASE(CELL_SPURS_TASK_ERROR_NOSYS);
		STR_CASE(CELL_SPURS_TASK_ERROR_NOMEM);
		STR_CASE(CELL_SPURS_TASK_ERROR_SRCH);
		STR_CASE(CELL_SPURS_TASK_ERROR_NOEXEC);
		STR_CASE(CELL_SPURS_TASK_ERROR_PERM);
		STR_CASE(CELL_SPURS_TASK_ERROR_BUSY);
		STR_CASE(CELL_SPURS_TASK_ERROR_FAULT);
		STR_CASE(CELL_SPURS_TASK_ERROR_ALIGN);
		STR_CASE(CELL_SPURS_TASK_ERROR_STAT);
		STR_CASE(CELL_SPURS_TASK_ERROR_NULL_POINTER);
		STR_CASE(CELL_SPURS_TASK_ERROR_FATAL);
		STR_CASE(CELL_SPURS_TASK_ERROR_SHUTDOWN);
		}

		return unknown;
	});
}

template <>
void fmt_class_string<CellSpursJobError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(CELL_SPURS_JOB_ERROR_AGAIN);
		STR_CASE(CELL_SPURS_JOB_ERROR_INVAL);
		STR_CASE(CELL_SPURS_JOB_ERROR_NOSYS);
		STR_CASE(CELL_SPURS_JOB_ERROR_NOMEM);
		STR_CASE(CELL_SPURS_JOB_ERROR_SRCH);
		STR_CASE(CELL_SPURS_JOB_ERROR_NOENT);
		STR_CASE(CELL_SPURS_JOB_ERROR_NOEXEC);
		STR_CASE(CELL_SPURS_JOB_ERROR_DEADLK);
		STR_CASE(CELL_SPURS_JOB_ERROR_PERM);
		STR_CASE(CELL_SPURS_JOB_ERROR_BUSY);
		STR_CASE(CELL_SPURS_JOB_ERROR_JOB_DESCRIPTOR);
		STR_CASE(CELL_SPURS_JOB_ERROR_JOB_DESCRIPTOR_SIZE);
		STR_CASE(CELL_SPURS_JOB_ERROR_FAULT);
		STR_CASE(CELL_SPURS_JOB_ERROR_CHILD);
		STR_CASE(CELL_SPURS_JOB_ERROR_STAT);
		STR_CASE(CELL_SPURS_JOB_ERROR_ALIGN);
		STR_CASE(CELL_SPURS_JOB_ERROR_NULL_POINTER);
		STR_CASE(CELL_SPURS_JOB_ERROR_MEMORY_CORRUPTED);
		STR_CASE(CELL_SPURS_JOB_ERROR_MEMORY_SIZE);
		STR_CASE(CELL_SPURS_JOB_ERROR_UNKNOWN_COMMAND);
		STR_CASE(CELL_SPURS_JOB_ERROR_JOBLIST_ALIGNMENT);
		STR_CASE(CELL_SPURS_JOB_ERROR_JOB_ALIGNMENT);
		STR_CASE(CELL_SPURS_JOB_ERROR_CALL_OVERFLOW);
		STR_CASE(CELL_SPURS_JOB_ERROR_ABORT);
		STR_CASE(CELL_SPURS_JOB_ERROR_DMALIST_ELEMENT);
		STR_CASE(CELL_SPURS_JOB_ERROR_NUM_CACHE);
		STR_CASE(CELL_SPURS_JOB_ERROR_INVALID_BINARY);
		}

		return unknown;
	});
}

template <>
void fmt_class_string<SpursWorkloadState>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		case SPURS_WKL_STATE_NON_EXISTENT: return "Non-existent";
		case SPURS_WKL_STATE_PREPARING: return "Preparing";
		case SPURS_WKL_STATE_RUNNABLE: return "Runnable";
		case SPURS_WKL_STATE_SHUTTING_DOWN: return "In-shutdown";
		case SPURS_WKL_STATE_REMOVABLE: return "Removable";
		case SPURS_WKL_STATE_INVALID: break;
		}

		return unknown;
	});
}

error_code sys_spu_image_close(ppu_thread&, vm::ptr<sys_spu_image> img);

//----------------------------------------------------------------------------
// Function prototypes
//----------------------------------------------------------------------------

bool spursKernelEntry(spu_thread& spu);

// SPURS Internals
namespace _spurs
{
	// Get the version of SDK used by this process
	s32 get_sdk_version();

	// Check whether libprof is loaded
	bool is_libprof_loaded();

	// Create an LV2 event queue and attach it to the SPURS instance
	s32 create_lv2_eq(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<u32> queueId, vm::ptr<u8> port, s32 size, const sys_event_queue_attribute_t& name);

	// Attach an LV2 event queue to the SPURS instance
	s32 attach_lv2_eq(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 queue, vm::ptr<u8> port, s32 isDynamic, bool spursCreated);

	// Detach an LV2 event queue from the SPURS instance
	s32 detach_lv2_eq(vm::ptr<CellSpurs> spurs, u8 spuPort, bool spursCreated);

	// Wait until a workload in the SPURS instance becomes ready
	void handler_wait_ready(ppu_thread& ppu, vm::ptr<CellSpurs> spurs);

	// Entry point of the SPURS handler thread. This thread is responsible for starting the SPURS SPU thread group.
	void handler_entry(ppu_thread& ppu, vm::ptr<CellSpurs> spurs);

	// Create the SPURS handler thread
	s32 create_handler(vm::ptr<CellSpurs> spurs, u32 ppuPriority);

	// Invoke event handlers
	s32 invoke_event_handlers(ppu_thread& ppu, vm::ptr<CellSpurs::EventPortMux> eventPortMux);

	// Invoke workload shutdown completion callbacks
	s32 wakeup_shutdown_completion_waiter(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 wid);

	// Entry point of the SPURS event helper thread
	void event_helper_entry(ppu_thread& ppu, vm::ptr<CellSpurs> spurs);

	// Create the SPURS event helper thread
	s32 create_event_helper(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 ppuPriority);

	// Initialise the event port multiplexor structure
	void init_event_port_mux(vm::ptr<CellSpurs::EventPortMux> eventPortMux, u8 spuPort, u32 eventPort, u32 unknown);

	// Enable the system workload
	s32 add_default_syswkl(vm::ptr<CellSpurs> spurs, vm::cptr<u8> swlPriority, u32 swlMaxSpu, u32 swlIsPreem);

	// Destroy the SPURS SPU threads and thread group
	s32 finalize_spu(ppu_thread&, vm::ptr<CellSpurs> spurs);

	// Stop the event helper thread
	s32 stop_event_helper(ppu_thread& ppu, vm::ptr<CellSpurs> spurs);

	// Signal to the SPURS handler thread
	s32 signal_to_handler_thread(ppu_thread& ppu, vm::ptr<CellSpurs> spurs);

	// Join the SPURS handler thread
	s32 join_handler_thread(ppu_thread& ppu, vm::ptr<CellSpurs> spurs);

	// Initialise SPURS
	s32 initialize(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 revision, u32 sdkVersion, s32 nSpus, s32 spuPriority, s32 ppuPriority, u32 flags, vm::cptr<char> prefix, u32 prefixSize, u32 container, vm::cptr<u8> swlPriority, u32 swlMaxSpu, u32 swlIsPreem);
}

//
// SPURS Core Functions
//

//s32 cellSpursInitialize(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, s32 nSpus, s32 spuPriority, s32 ppuPriority, b8 exitIfNoWork);
//s32 cellSpursInitializeWithAttribute(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::cptr<CellSpursAttribute> attr);
//s32 cellSpursInitializeWithAttribute2(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::cptr<CellSpursAttribute> attr);
//s32 _cellSpursAttributeInitialize(vm::ptr<CellSpursAttribute> attr, u32 revision, u32 sdkVersion, u32 nSpus, s32 spuPriority, s32 ppuPriority, b8 exitIfNoWork);
//s32 cellSpursAttributeSetMemoryContainerForSpuThread(vm::ptr<CellSpursAttribute> attr, u32 container);
//s32 cellSpursAttributeSetNamePrefix(vm::ptr<CellSpursAttribute> attr, vm::cptr<char> prefix, u32 size);
//s32 cellSpursAttributeEnableSpuPrintfIfAvailable(vm::ptr<CellSpursAttribute> attr);
//s32 cellSpursAttributeSetSpuThreadGroupType(vm::ptr<CellSpursAttribute> attr, s32 type);
//s32 cellSpursAttributeEnableSystemWorkload(vm::ptr<CellSpursAttribute> attr, vm::cptr<u8[8]> priority, u32 maxSpu, vm::cptr<b8[8]> isPreemptible);
//s32 cellSpursFinalize(vm::ptr<CellSpurs> spurs);
//s32 cellSpursGetSpuThreadGroupId(vm::ptr<CellSpurs> spurs, vm::ptr<u32> group);
//s32 cellSpursGetNumSpuThread(vm::ptr<CellSpurs> spurs, vm::ptr<u32> nThreads);
//s32 cellSpursGetSpuThreadId(vm::ptr<CellSpurs> spurs, vm::ptr<u32> thread, vm::ptr<u32> nThreads);
//s32 cellSpursSetMaxContention(vm::ptr<CellSpurs> spurs, u32 wid, u32 maxContention);
//s32 cellSpursSetPriorities(vm::ptr<CellSpurs> spurs, u32 wid, vm::cptr<u8> priorities);
//s32 cellSpursSetPreemptionVictimHints(vm::ptr<CellSpurs> spurs, vm::cptr<b8> isPreemptible);
//s32 cellSpursAttachLv2EventQueue(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 queue, vm::ptr<u8> port, s32 isDynamic);
//s32 cellSpursDetachLv2EventQueue(vm::ptr<CellSpurs> spurs, u8 port);

// Enable the SPU exception event handler
s32 cellSpursEnableExceptionEventHandler(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, b8 flag);

//s32 cellSpursSetGlobalExceptionEventHandler(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursGlobalExceptionEventHandler> eaHandler, vm::ptr<void> arg);
//s32 cellSpursUnsetGlobalExceptionEventHandler(vm::ptr<CellSpurs> spurs);
//s32 cellSpursGetInfo(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursInfo> info);

//
// SPURS SPU GUID functions
//

//s32 cellSpursGetSpuGuid();

//
// SPURS trace functions
//

namespace _spurs
{
	// Signal SPUs to update trace status
	void trace_status_update(ppu_thread& ppu, vm::ptr<CellSpurs> spurs);

	// Initialize SPURS trace
	s32 trace_initialize(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTraceInfo> buffer, u32 size, u32 mode, u32 updateStatus);

	// Start SPURS trace
	s32 trace_start(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 updateStatus);

	// Stop SPURS trace
	s32 trace_stop(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 updateStatus);
}

//s32 cellSpursTraceInitialize(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTraceInfo> buffer, u32 size, u32 mode);
//s32 cellSpursTraceFinalize(ppu_thread& ppu, vm::ptr<CellSpurs> spurs);
//s32 cellSpursTraceStart(ppu_thread& ppu, vm::ptr<CellSpurs> spurs);
//s32 cellSpursTraceStop(ppu_thread& ppu, vm::ptr<CellSpurs> spurs);

//
// SPURS policy module functions
//

namespace _spurs
{
	// Add workload
	s32 add_workload(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<u32> wid, vm::cptr<void> pm, u32 size, u64 data, const u8(&priorityTable)[8], u32 minContention, u32 maxContention, vm::cptr<char> nameClass, vm::cptr<char> nameInstance, vm::ptr<CellSpursShutdownCompletionEventHook> hook, vm::ptr<void> hookArg);
}

//s32 _cellSpursWorkloadAttributeInitialize(vm::ptr<CellSpursWorkloadAttribute> attr, u32 revision, u32 sdkVersion, vm::cptr<void> pm, u32 size, u64 data, vm::cptr<u8[8]> priority, u32 minCnt, u32 maxCnt);
//s32 cellSpursWorkloadAttributeSetName(vm::ptr<CellSpursWorkloadAttribute> attr, vm::cptr<char> nameClass, vm::cptr<char> nameInstance);
//s32 cellSpursWorkloadAttributeSetShutdownCompletionEventHook(vm::ptr<CellSpursWorkloadAttribute> attr, vm::ptr<CellSpursShutdownCompletionEventHook> hook, vm::ptr<void> arg);
//s32 cellSpursAddWorkload(vm::ptr<CellSpurs> spurs, vm::ptr<u32> wid, vm::cptr<void> pm, u32 size, u64 data, vm::cptr<u8[8]> priority, u32 minCnt, u32 maxCnt);
//s32 cellSpursAddWorkloadWithAttribute(vm::ptr<CellSpurs> spurs, vm::ptr<u32> wid, vm::cptr<CellSpursWorkloadAttribute> attr);
//s32 cellSpursShutdownWorkload();
//s32 cellSpursWaitForWorkloadShutdown();
//s32 cellSpursRemoveWorkload();

// Activate the SPURS kernel
s32 cellSpursWakeUp(ppu_thread& ppu, vm::ptr<CellSpurs> spurs);

s32 cellSpursSendWorkloadSignal(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 wid);
//s32 cellSpursGetWorkloadFlag(vm::ptr<CellSpurs> spurs, vm::pptr<CellSpursWorkloadFlag> flag);
s32 cellSpursReadyCountStore(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 wid, u32 value);
s32 cellSpursReadyCountSwap(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 wid, vm::ptr<u32> old, u32 swap);
s32 cellSpursReadyCountCompareAndSwap(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 wid, vm::ptr<u32> old, u32 compare, u32 swap);
s32 cellSpursReadyCountAdd(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 wid, vm::ptr<u32> old, s32 value);
//s32 cellSpursGetWorkloadData(vm::ptr<CellSpurs> spurs, vm::ptr<u64> data, u32 wid);
//s32 cellSpursGetWorkloadInfo();
//s32 cellSpursSetExceptionEventHandler();
//s32 cellSpursUnsetExceptionEventHandler();
s32 _cellSpursWorkloadFlagReceiver(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 wid, u32 is_set);
//s32 _cellSpursWorkloadFlagReceiver2();
//error_code cellSpursRequestIdleSpu();

//
// SPURS taskset functions
//

namespace _spurs
{
	// Create taskset
	s32 create_taskset(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, u64 args, vm::cptr<u8[8]> priority, u32 max_contention, vm::cptr<char> name, u32 size, s32 enable_clear_ls);
}

//s32 cellSpursCreateTasksetWithAttribute(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursTasksetAttribute> attr);
//s32 cellSpursCreateTaskset(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, u64 args, vm::cptr<u8[8]> priority, u32 maxContention);
//s32 cellSpursJoinTaskset(vm::ptr<CellSpursTaskset> taskset);
//s32 cellSpursGetTasksetId(vm::ptr<CellSpursTaskset> taskset, vm::ptr<u32> wid);
//s32 cellSpursShutdownTaskset(vm::ptr<CellSpursTaskset> taskset);
//s32 cellSpursTasksetAttributeSetName(vm::ptr<CellSpursTasksetAttribute> attr, vm::cptr<char> name);
//s32 cellSpursTasksetAttributeSetTasksetSize(vm::ptr<CellSpursTasksetAttribute> attr, u32 size);
//s32 cellSpursTasksetAttributeEnableClearLS(vm::ptr<CellSpursTasksetAttribute> attr, s32 enable);
//s32 _cellSpursTasksetAttribute2Initialize(vm::ptr<CellSpursTasksetAttribute2> attribute, u32 revision);
//s32 cellSpursCreateTaskset2(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursTasksetAttribute2> attr);
//s32 cellSpursDestroyTaskset2();
//s32 cellSpursTasksetSetExceptionEventHandler(vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursTasksetExceptionEventHandler> handler, vm::ptr<u64> arg);
//s32 cellSpursTasksetUnsetExceptionEventHandler(vm::ptr<CellSpursTaskset> taskset);

// Get taskset instance from the workload ID
s32 cellSpursLookUpTasksetAddress(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::pptr<CellSpursTaskset> taskset, u32 id);

//s32 cellSpursTasksetGetSpursAddress(vm::cptr<CellSpursTaskset> taskset, vm::ptr<u32> spurs);
//s32 cellSpursGetTasksetInfo();
//s32 _cellSpursTasksetAttributeInitialize(vm::ptr<CellSpursTasksetAttribute> attribute, u32 revision, u32 sdk_version, u64 args, vm::cptr<u8> priority, u32 max_contention);

//
// SPURS task functions
//

namespace _spurs
{
	// Create task
	s32 create_task(vm::ptr<CellSpursTaskset> taskset, vm::ptr<u32> task_id, vm::cptr<void> elf, vm::cptr<void> context, u32 size, vm::ptr<CellSpursTaskLsPattern> ls_pattern, vm::ptr<CellSpursTaskArgument> arg);

	// Start task
	s32 task_start(ppu_thread& ppu, vm::ptr<CellSpursTaskset> taskset, u32 taskId);
}

//s32 cellSpursCreateTask(ppu_thread& ppu, vm::ptr<CellSpursTaskset> taskset, vm::ptr<u32> taskId, vm::cptr<void> elf, vm::cptr<void> context, u32 size, vm::ptr<CellSpursTaskLsPattern> lsPattern, vm::ptr<CellSpursTaskArgument> argument);

// Sends a signal to the task
s32 _cellSpursSendSignal(ppu_thread& ppu, vm::ptr<CellSpursTaskset> taskset, u32 taskId);

//s32 cellSpursCreateTaskWithAttribute();
//s32 cellSpursTaskExitCodeGet();
//s32 cellSpursTaskExitCodeInitialize();
//s32 cellSpursTaskExitCodeTryGet();
//s32 cellSpursTaskGetLoadableSegmentPattern();
//s32 cellSpursTaskGetReadOnlyAreaPattern();
//s32 cellSpursTaskGenerateLsPattern();
//s32 _cellSpursTaskAttributeInitialize();
//s32 cellSpursTaskAttributeSetExitCodeContainer();
//s32 _cellSpursTaskAttribute2Initialize(vm::ptr<CellSpursTaskAttribute2> attribute, u32 revision);
//s32 cellSpursTaskGetContextSaveAreaSize();
//s32 cellSpursCreateTask2();
//s32 cellSpursJoinTask2();
//s32 cellSpursTryJoinTask2();
//s32 cellSpursCreateTask2WithBinInfo();

//
// SPURS event flag functions
//

namespace _spurs
{
	// Wait for SPURS event flag
	s32 event_flag_wait(ppu_thread& ppu, vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u16> mask, u32 mode, u32 block);
}

//s32 _cellSpursEventFlagInitialize(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursEventFlag> eventFlag, u32 flagClearMode, u32 flagDirection);
//s32 cellSpursEventFlagClear(vm::ptr<CellSpursEventFlag> eventFlag, u16 bits);
//s32 cellSpursEventFlagSet(ppu_thread& ppu, vm::ptr<CellSpursEventFlag> eventFlag, u16 bits);
//s32 cellSpursEventFlagWait(ppu_thread& ppu, vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u16> mask, u32 mode);
//s32 cellSpursEventFlagTryWait(ppu_thread& ppu, vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u16> mask, u32 mode);
//s32 cellSpursEventFlagAttachLv2EventQueue(ppu_thread& ppu, vm::ptr<CellSpursEventFlag> eventFlag);
//s32 cellSpursEventFlagDetachLv2EventQueue(ppu_thread& ppu, vm::ptr<CellSpursEventFlag> eventFlag);
//s32 cellSpursEventFlagGetDirection(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u32> direction);
//s32 cellSpursEventFlagGetClearMode(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u32> clear_mode);
//s32 cellSpursEventFlagGetTasksetAddress(vm::ptr<CellSpursEventFlag> eventFlag, vm::pptr<CellSpursTaskset> taskset);

//
// SPURS lock free queue functions
//

//s32 _cellSpursLFQueueInitialize(vm::ptr<void> pTasksetOrSpurs, vm::ptr<CellSpursLFQueue> pQueue, vm::cptr<void> buffer, u32 size, u32 depth, u32 direction);
//s32 _cellSpursLFQueuePushBody();
//s32 cellSpursLFQueueAttachLv2EventQueue(vm::ptr<CellSyncLFQueue> queue);
//s32 cellSpursLFQueueDetachLv2EventQueue(vm::ptr<CellSyncLFQueue> queue);
//s32 _cellSpursLFQueuePopBody();
//s32 cellSpursLFQueueGetTasksetAddress();

//
// SPURS queue functions
//

//s32 _cellSpursQueueInitialize();
//s32 cellSpursQueuePopBody();
//s32 cellSpursQueuePushBody();
//s32 cellSpursQueueAttachLv2EventQueue();
//s32 cellSpursQueueDetachLv2EventQueue();
//s32 cellSpursQueueGetTasksetAddress();
//s32 cellSpursQueueClear();
//s32 cellSpursQueueDepth();
//s32 cellSpursQueueGetEntrySize();
//s32 cellSpursQueueSize();
//s32 cellSpursQueueGetDirection();

//
// SPURS barrier functions
//

//s32 cellSpursBarrierInitialize();
//s32 cellSpursBarrierGetTasksetAddress();

//
// SPURS semaphore functions
//

//s32 _cellSpursSemaphoreInitialize();
//s32 cellSpursSemaphoreGetTasksetAddress();

//
// SPURS job chain functions
//

namespace _spurs
{
	s32 check_job_chain_attribute(u32 sdkVer, vm::cptr<u64> jcEntry, u16 sizeJobDescr, u16 maxGrabbedJob
		, u64 priorities, u32 maxContention, u8 autoSpuCount, u32 tag1, u32 tag2
		, u8 isFixedMemAlloc, u32 maxSizeJob, u32 initSpuCount);

	s32 create_job_chain(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursJobChain> jobChain, vm::cptr<u64> jobChainEntry, u16 sizeJob
		, u16 maxGrabbedJob, vm::cptr<u8[8]> prio, u32 maxContention, b8 autoReadyCount
		, u32 tag1, u32 tag2, u32 HaltOnError, vm::cptr<char> name, u32 param_13, u32 param_14);

}

//s32 cellSpursCreateJobChainWithAttribute();
//s32 cellSpursCreateJobChain();
//s32 cellSpursJoinJobChain();
s32 cellSpursKickJobChain(ppu_thread& ppu, vm::ptr<CellSpursJobChain> jobChain, u8 numReadyCount);
//s32 _cellSpursJobChainAttributeInitialize();
//s32 cellSpursGetJobChainId();
//s32 cellSpursJobChainSetExceptionEventHandler();
//s32 cellSpursJobChainUnsetExceptionEventHandler();
//s32 cellSpursGetJobChainInfo();
//s32 cellSpursJobChainGetSpursAddress();
//s32 cellSpursJobGuardInitialize();
//s32 cellSpursJobChainAttributeSetName();
//s32 cellSpursShutdownJobChain();
//s32 cellSpursJobChainAttributeSetHaltOnError();
//s32 cellSpursJobChainAttributeSetJobTypeMemoryCheck();
//s32 cellSpursJobGuardNotify();
//s32 cellSpursJobGuardReset();
s32 cellSpursRunJobChain(ppu_thread& ppu, vm::ptr<CellSpursJobChain> jobChain);
//s32 cellSpursJobChainGetError();
//s32 cellSpursGetJobPipelineInfo();
//s32 cellSpursJobSetMaxGrab();
//s32 cellSpursJobHeaderSetJobbin2Param();
//s32 cellSpursAddUrgentCommand();
//s32 cellSpursAddUrgentCall();

//----------------------------------------------------------------------------
// SPURS utility functions
//----------------------------------------------------------------------------

s32 _spurs::get_sdk_version()
{
	const s32 version = static_cast<s32>(g_ps3_process_info.sdk_ver);
	return version == -1 ? 0x485000 : version;
}

bool _spurs::is_libprof_loaded()
{
	return false;
}

//----------------------------------------------------------------------------
// SPURS core functions
//----------------------------------------------------------------------------

s32 _spurs::create_lv2_eq(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<u32> queueId, vm::ptr<u8> port, s32 size, const sys_event_queue_attribute_t& attr)
{
	if (s32 rc = sys_event_queue_create(ppu, queueId, vm::make_var(attr), SYS_EVENT_QUEUE_LOCAL, size))
	{
		static_cast<void>(ppu.test_stopped());
		return rc;
	}

	if (_spurs::attach_lv2_eq(ppu, spurs, *queueId, port, 1, true))
	{
		sys_event_queue_destroy(ppu, *queueId, SYS_EVENT_QUEUE_DESTROY_FORCE);
		static_cast<void>(ppu.test_stopped());
	}

	return CELL_OK;
}

s32 _spurs::attach_lv2_eq(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 queue, vm::ptr<u8> port, s32 isDynamic, bool spursCreated)
{
	if (!spurs || !port)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (spurs->exception)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	u8 _port     = 0x3f;
	u64 portMask = 0;

	if (isDynamic == 0)
	{
		_port = *port;
		if (_port > 0x3f)
		{
			return CELL_SPURS_CORE_ERROR_INVAL;
		}

		if (_spurs::get_sdk_version() >= 0x180000 && _port > 0xf)
		{
			return CELL_SPURS_CORE_ERROR_PERM;
		}
	}

	for (u32 i = isDynamic ? 0x10 : _port; i <= _port; i++)
	{
		portMask |= 1ull << (i);
	}

	if (s32 res = sys_spu_thread_group_connect_event_all_threads(ppu, spurs->spuTG, queue, portMask, port))
	{
		if (res + 0u == CELL_EISCONN)
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

s32 _spurs::detach_lv2_eq(vm::ptr<CellSpurs> spurs, u8 spuPort, bool spursCreated)
{
	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spursCreated && spurs->exception)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	if (spuPort > 0x3F)
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	if (!spursCreated)
	{
		if (!spurs->spuPortBits.bit_test_reset(spuPort) && _spurs::get_sdk_version() >= 0x180000)
		{
			return CELL_SPURS_CORE_ERROR_SRCH;
		}
	}

	return CELL_OK;
}

void _spurs::handler_wait_ready(ppu_thread& ppu, vm::ptr<CellSpurs> spurs)
{
	ensure(ppu_execute<&sys_lwmutex_lock>(ppu, spurs.ptr(&CellSpurs::mutex), 0) == 0);
	static_cast<void>(ppu.test_stopped());

	while (true)
	{
		if (spurs->handlerExiting)
		{
			ensure(ppu_execute<&sys_lwmutex_unlock>(ppu, spurs.ptr(&CellSpurs::mutex)) == 0);

			return sys_ppu_thread_exit(ppu, 0);
		}

		// Find a runnable workload
		spurs->handlerDirty = 0;
		if (spurs->exception == 0u)
		{
			bool foundRunnableWorkload = false;
			for (u32 i = 0; i < 16; i++)
			{
				if (spurs->wklState1[i] == SPURS_WKL_STATE_RUNNABLE &&
					std::bit_cast<u64>(spurs->wklInfo1[i].priority) != 0 &&
					spurs->wklMaxContention[i] & 0x0F)
				{
					if (spurs->wklReadyCount1[i] ||
						spurs->wklSignal1.load() & (0x8000u >> i) ||
						(spurs->wklFlag.flag.load() == 0u &&
							spurs->wklFlagReceiver == static_cast<u8>(i)))
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
					if (spurs->wklState2[i] == SPURS_WKL_STATE_RUNNABLE &&
						std::bit_cast<u64>(spurs->wklInfo2[i].priority) != 0 &&
						spurs->wklMaxContention[i] & 0xF0)
					{
						if (spurs->wklIdleSpuCountOrReadyCount2[i] ||
							spurs->wklSignal2.load() & (0x8000u >> i) ||
							(spurs->wklFlag.flag.load() == 0u &&
								spurs->wklFlagReceiver == static_cast<u8>(i) + 0x10))
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
		spurs->handlerWaiting = 1;
		if (spurs->handlerDirty == 0)
		{
			ensure(ppu_execute<&sys_lwcond_wait>(ppu, spurs.ptr(&CellSpurs::cond), 0) == 0);
			static_cast<void>(ppu.test_stopped());
		}

		spurs->handlerWaiting = 0;
	}

	// If we reach here then a runnable workload was found
	ensure(ppu_execute<&sys_lwmutex_unlock>(ppu, spurs.ptr(&CellSpurs::mutex)) == 0);
	static_cast<void>(ppu.test_stopped());
}

void _spurs::handler_entry(ppu_thread& ppu, vm::ptr<CellSpurs> spurs)
{
	if (spurs->flags & SAF_UNKNOWN_FLAG_30)
	{
		return sys_ppu_thread_exit(ppu, 0);
	}

	while (true)
	{
		if (spurs->flags1 & SF1_EXIT_IF_NO_WORK)
		{
			_spurs::handler_wait_ready(ppu, spurs);
		}

		ensure(sys_spu_thread_group_start(ppu, spurs->spuTG) == 0);

		const s32 rc = sys_spu_thread_group_join(ppu, spurs->spuTG, vm::null, vm::null);
		static_cast<void>(ppu.test_stopped());

		if (rc + 0u != CELL_EFAULT)
		{
			if (rc + 0u == CELL_ESTAT)
			{
				return sys_ppu_thread_exit(ppu, 0);
			}

			ensure(rc + 0u == CELL_EFAULT);
		}

		if ((spurs->flags1 & SF1_EXIT_IF_NO_WORK) == 0)
		{
			ensure((spurs->handlerExiting == 1));

			return sys_ppu_thread_exit(ppu, 0);
		}
	}
}

s32 _spurs::create_handler(vm::ptr<CellSpurs> /*spurs*/, u32 /*ppuPriority*/)
{
	struct handler_thread : ppu_thread
	{
		using ppu_thread::ppu_thread;

		void non_task()
		{
			//BIND_FUNC(_spurs::handler_entry)(*this);
		}
	};

	// auto eht = idm::make_ptr<ppu_thread, handler_thread>(std::string(spurs->prefix, spurs->prefixSize) + "SpursHdlr0", ppuPriority, 0x4000);

	// spurs->ppu0 = eht->id;

	// eht->gpr[3] = spurs.addr();
	// eht->run();

	return CELL_OK;
}

s32 _spurs::invoke_event_handlers(ppu_thread& ppu, vm::ptr<CellSpurs::EventPortMux> eventPortMux)
{
	if (eventPortMux->reqPending.exchange(0))
	{
		for (auto node = eventPortMux->handlerList.exchange(vm::null); node; node = node->next)
		{
			node->handler(ppu, eventPortMux, node->data);
		}
	}

	return CELL_OK;
}

s32 _spurs::wakeup_shutdown_completion_waiter(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 wid)
{
	if (!spurs)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (wid >= spurs->max_workloads())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}

	if ((spurs->wklEnabled.load() & (0x80000000u >> wid)) == 0u)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_SRCH;
	}

	if (spurs->wklState(wid) != SPURS_WKL_STATE_REMOVABLE)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	const auto wklF     = wid < CELL_SPURS_MAX_WORKLOAD ? &spurs->wklF1[wid]     : &spurs->wklF2[wid & 0x0F];
	const auto wklEvent = &spurs->wklEvent(wid);

	if (wklF->hook)
	{
		wklF->hook(ppu, spurs, wid, wklF->hookArg);

		ensure((wklEvent->load() & 0x01));
		ensure((wklEvent->load() & 0x02));
		ensure((wklEvent->load() & 0x20) == 0);
		wklEvent->fetch_or(0x20);
	}

	s32 rc = CELL_OK;
	if (!wklF->hook || wklEvent->load() & 0x10)
	{
		ensure((wklF->x28 == 2u));
		rc = sys_semaphore_post(ppu, static_cast<u32>(wklF->sem), 1);
		static_cast<void>(ppu.test_stopped());
	}

	return rc;
}

void _spurs::event_helper_entry(ppu_thread& ppu, vm::ptr<CellSpurs> spurs)
{
	vm::var<sys_event_t[]> events(8);
	vm::var<u32> count;

	while (true)
	{
		ensure(sys_event_queue_receive(ppu, spurs->eventQueue, vm::null, 0) == 0);
		static_cast<void>(ppu.test_stopped());

		const u64 event_src   = ppu.gpr[4];
		const u64 event_data1 = ppu.gpr[5];
		const u64 event_data2 = ppu.gpr[6];
		const u64 event_data3 = ppu.gpr[7];

		if (event_src == SYS_SPU_THREAD_EVENT_EXCEPTION_KEY)
		{
			spurs->exception = 1;

			events[0].source = event_src;
			events[0].data1 = event_data1;
			events[0].data2 = event_data2;
			events[0].data3 = event_data3;

			if (sys_event_queue_tryreceive(ppu, spurs->eventQueue, events + 1, 7, count) != CELL_OK)
			{
				continue;
			}

			// TODO: Examine LS and dump exception details

			for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++)
			{
				sys_semaphore_post(ppu, static_cast<u32>(spurs->wklF1[i].sem), 1);

				if (spurs->flags1 & SF1_32_WORKLOADS)
				{
					sys_semaphore_post(ppu, static_cast<u32>(spurs->wklF2[i].sem), 1);
				}
			}

			static_cast<void>(ppu.test_stopped());
		}
		else
		{
			const u32 data0 = event_data2 & 0x00FFFFFF;

			if (data0 == 1)
			{
				return;
			}
			else if (data0 < 1)
			{
				const u32 shutdownMask = static_cast<u32>(event_data3);

				for (u32 wid = 0; wid < CELL_SPURS_MAX_WORKLOAD; wid++)
				{
					if (shutdownMask & (0x80000000u >> wid))
					{
						ensure(_spurs::wakeup_shutdown_completion_waiter(ppu, spurs, wid) == 0);
					}

					if ((spurs->flags1 & SF1_32_WORKLOADS) && (shutdownMask & (0x8000 >> wid)))
					{
						ensure(_spurs::wakeup_shutdown_completion_waiter(ppu, spurs, wid + 0x10) == 0);
					}
				}
			}
			else if (data0 == 2)
			{
				ensure(sys_semaphore_post(ppu, static_cast<u32>(spurs->semPrv), 1) == 0);
				static_cast<void>(ppu.test_stopped());
			}
			else if (data0 == 3)
			{
				ensure(_spurs::invoke_event_handlers(ppu, spurs.ptr(&CellSpurs::eventPortMux)) == 0);
			}
			else
			{
				fmt::throw_exception("data0=0x%x", data0);
			}
		}
	}
}

s32 _spurs::create_event_helper(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 /*ppuPriority*/)
{
	if (s32 rc = _spurs::create_lv2_eq(ppu, spurs, spurs.ptr(&CellSpurs::eventQueue), spurs.ptr(&CellSpurs::spuPort), 0x2A, sys_event_queue_attribute_t{SYS_SYNC_PRIORITY, SYS_PPU_QUEUE, {"_spuPrv\0"_u64}}))
	{
		return rc;
	}

	if (sys_event_port_create(ppu, spurs.ptr(&CellSpurs::eventPort), SYS_EVENT_PORT_LOCAL, SYS_EVENT_PORT_NO_NAME))
	{
		if (_spurs::detach_lv2_eq(spurs, spurs->spuPort, true))
		{
			return CELL_SPURS_CORE_ERROR_AGAIN;
		}

		sys_event_queue_destroy(ppu, spurs->eventQueue, SYS_EVENT_QUEUE_DESTROY_FORCE);
		return CELL_SPURS_CORE_ERROR_AGAIN;
	}

	if (sys_event_port_connect_local(ppu, spurs->eventPort, spurs->eventQueue))
	{
		sys_event_port_destroy(ppu, spurs->eventPort);

		if (_spurs::detach_lv2_eq(spurs, spurs->spuPort, true))
		{
			return CELL_SPURS_CORE_ERROR_STAT;
		}

		sys_event_queue_destroy(ppu, spurs->eventQueue, SYS_EVENT_QUEUE_DESTROY_FORCE);
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	struct event_helper_thread : ppu_thread
	{
		using ppu_thread::ppu_thread;

		void non_task()
		{
			//BIND_FUNC(_spurs::event_helper_entry)(*this);
		}
	};

	//auto eht = idm::make_ptr<ppu_thread, event_helper_thread>(std::string(spurs->prefix, spurs->prefixSize) + "SpursHdlr1", ppuPriority, 0x8000);

	//if (!eht)
	{
		sys_event_port_disconnect(ppu, spurs->eventPort);
		sys_event_port_destroy(ppu, spurs->eventPort);

		if (_spurs::detach_lv2_eq(spurs, spurs->spuPort, true))
		{
			return CELL_SPURS_CORE_ERROR_STAT;
		}

		sys_event_queue_destroy(ppu, spurs->eventQueue, SYS_EVENT_QUEUE_DESTROY_FORCE);
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	// eht->gpr[3] = spurs.addr();
	// eht->run();

	// spurs->ppu1 = eht->id;
	return CELL_OK;
}

void _spurs::init_event_port_mux(vm::ptr<CellSpurs::EventPortMux> eventPortMux, u8 spuPort, u32 eventPort, u32 unknown)
{
	memset(eventPortMux.get_ptr(), 0, sizeof(CellSpurs::EventPortMux));
	eventPortMux->spuPort   = spuPort;
	eventPortMux->eventPort = eventPort;
	eventPortMux->x08       = unknown;
}

s32 _spurs::add_default_syswkl(vm::ptr<CellSpurs> /*spurs*/, vm::cptr<u8> /*swlPriority*/, u32 /*swlMaxSpu*/, u32 /*swlIsPreem*/)
{
	// TODO: Implement this
	return CELL_OK;
}

s32 _spurs::finalize_spu(ppu_thread& ppu, vm::ptr<CellSpurs> spurs)
{
	if (spurs->flags & SAF_UNKNOWN_FLAG_7 || spurs->flags & SAF_UNKNOWN_FLAG_8)
	{
		while (true)
		{
			ensure(sys_spu_thread_group_join(ppu, spurs->spuTG, vm::null, vm::null) + 0u == CELL_EFAULT);

			if (s32 rc = sys_spu_thread_group_destroy(ppu, spurs->spuTG))
			{
				if (rc + 0u == CELL_EBUSY)
				{
					continue;
				}

				ensure(rc == CELL_OK);
			}

			break;
		}
	}
	else
	{
		if (s32 rc = sys_spu_thread_group_destroy(ppu, spurs->spuTG))
		{
			return rc;
		}
	}

	ensure(ppu_execute<&sys_spu_image_close>(ppu, spurs.ptr(&CellSpurs::spuImg)) == 0);

	return CELL_OK;
}

s32 _spurs::stop_event_helper(ppu_thread& ppu, vm::ptr<CellSpurs> spurs)
{
	if (spurs->ppu1 == 0xFFFFFFFF)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	if (sys_event_port_send(spurs->eventPort, 0, 1, 0) != CELL_OK)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	if (sys_ppu_thread_join(ppu, static_cast<u32>(spurs->ppu1), vm::var<u64>{}) != CELL_OK)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	spurs->ppu1 = 0xFFFFFFFF;

	ensure(sys_event_port_disconnect(ppu, spurs->eventPort) == 0);
	ensure(sys_event_port_destroy(ppu, spurs->eventPort) == 0);
	ensure(_spurs::detach_lv2_eq(spurs, spurs->spuPort, true) == 0);
	ensure(sys_event_queue_destroy(ppu, spurs->eventQueue, SYS_EVENT_QUEUE_DESTROY_FORCE) == 0);

	return CELL_OK;
}

s32 _spurs::signal_to_handler_thread(ppu_thread& ppu, vm::ptr<CellSpurs> spurs)
{
	ensure(ppu_execute<&sys_lwmutex_lock>(ppu, spurs.ptr(&CellSpurs::mutex), 0) == 0);
	ensure(ppu_execute<&sys_lwcond_signal>(ppu, spurs.ptr(&CellSpurs::cond)) == 0);
	ensure(ppu_execute<&sys_lwmutex_unlock>(ppu, spurs.ptr(&CellSpurs::mutex)) == 0);

	return CELL_OK;
}

s32 _spurs::join_handler_thread(ppu_thread& ppu, vm::ptr<CellSpurs> spurs)
{
	if (spurs->ppu0 == 0xFFFFFFFF)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	ensure(sys_ppu_thread_join(ppu, static_cast<u32>(spurs->ppu0), vm::var<u64>{}) == 0);

	spurs->ppu0 = 0xFFFFFFFF;
	return CELL_OK;
}

s32 _spurs::initialize(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 revision, u32 sdkVersion, s32 nSpus, s32 spuPriority, s32 ppuPriority, u32 flags, vm::cptr<char> prefix, u32 prefixSize, u32 container, vm::cptr<u8> swlPriority, u32 swlMaxSpu, u32 swlIsPreem)
{
	vm::var<u32>                            sem;
	vm::var<sys_semaphore_attribute_t>      semAttr;
	vm::var<char[]>                         spuTgName(128);
	vm::var<sys_spu_thread_group_attribute> spuTgAttr;
	vm::var<sys_spu_thread_argument>        spuThArgs;
	vm::var<sys_spu_thread_attribute>       spuThAttr;
	vm::var<char[]>                         spuThName(128);

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

	if (process_is_spu_lock_line_reservation_address(spurs.addr(), SYS_MEMORY_ACCESS_RIGHT_SPU_THR))
	{
		return CELL_SPURS_CORE_ERROR_PERM;
	}

	// Intialise SPURS context
	const bool isSecond = (flags & SAF_SECOND_VERSION) != 0;

	auto rollback = [&]
	{
		if (spurs->semPrv)
		{
			sys_semaphore_destroy(ppu, ::narrow<u32>(+spurs->semPrv));
		}

		for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++)
		{
			if (spurs->wklF1[i].sem)
			{
				sys_semaphore_destroy(ppu, ::narrow<u32>(+spurs->wklF1[i].sem));
			}

			if (isSecond)
			{
				if (spurs->wklF2[i].sem)
				{
					sys_semaphore_destroy(ppu, ::narrow<u32>(+spurs->wklF2[i].sem));
				}
			}
		}
	};

	std::memset(spurs.get_ptr(), 0, isSecond ? CELL_SPURS_SIZE2 : CELL_SPURS_SIZE);
	spurs->revision   = revision;
	spurs->sdkVersion = sdkVersion;
	spurs->ppu0       = 0xffffffffull;
	spurs->ppu1       = 0xffffffffull;
	spurs->flags      = flags;
	spurs->prefixSize = static_cast<u8>(prefixSize);
	std::memcpy(spurs->prefix, prefix.get_ptr(), prefixSize);

	if (!isSecond)
	{
		spurs->wklEnabled = 0xffff;
	}

	// Initialise trace
	spurs->sysSrvTrace.store({});

	for (u32 i = 0; i < 8; i++)
	{
		spurs->sysSrvPreemptWklId[i] = -1;
	}

	// Import default system workload
	spurs->wklInfoSysSrv.addr.set(SPURS_IMG_ADDR_SYS_SRV_WORKLOAD);
	spurs->wklInfoSysSrv.size = 0x2200;
	spurs->wklInfoSysSrv.arg  = 0;
	spurs->wklInfoSysSrv.uniqueId = 0xff;

	// Create semaphores for each workload
	semAttr->protocol = SYS_SYNC_PRIORITY;
	semAttr->pshared  = SYS_SYNC_NOT_PROCESS_SHARED;
	semAttr->ipc_key  = 0;
	semAttr->flags    = 0;
	semAttr->name_u64 = "_spuWkl\0"_u64;

	for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++)
	{
		if (s32 rc = sys_semaphore_create(ppu, sem, semAttr, 0, 1))
		{
			return rollback(), rc;
		}

		spurs->wklF1[i].sem = *sem;

		if (isSecond)
		{
			if (s32 rc = sys_semaphore_create(ppu, sem, semAttr, 0, 1))
			{
				return rollback(), rc;
			}

			spurs->wklF2[i].sem = *sem;
		}
	}

	// Create semaphore
	semAttr->name_u64 = "_spuPrv\0"_u64;
	if (s32 rc = sys_semaphore_create(ppu, sem, semAttr, 0, 1))
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
	spurs->spuImg.segs        = vm::null;
	spurs->spuImg.entry_point = isSecond ? CELL_SPURS_KERNEL2_ENTRY_ADDR : CELL_SPURS_KERNEL1_ENTRY_ADDR;
	spurs->spuImg.nsegs       = 0;

	// Create a thread group for this SPURS context
	std::memcpy(spuTgName.get_ptr(), spurs->prefix, spurs->prefixSize);
	std::memcpy(spuTgName.get_ptr() + spurs->prefixSize, "CellSpursKernelGroup", 21);

	spuTgAttr->name  = spuTgName;
	spuTgAttr->nsize = static_cast<u32>(std::strlen(spuTgAttr->name.get_ptr())) + 1;
	spuTgAttr->type  = SYS_SPU_THREAD_GROUP_TYPE_NORMAL;

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

	if (s32 rc = sys_spu_thread_group_create(ppu, spurs.ptr(&CellSpurs::spuTG), nSpus, spuPriority, vm::unsafe_ptr_cast<reduced_sys_spu_thread_group_attribute>(spuTgAttr)))
	{
		ppu_execute<&sys_spu_image_close>(ppu, spurs.ptr(&CellSpurs::spuImg));
		return rollback(), rc;
	}

	// Initialise all SPUs in the SPU thread group
	std::memcpy(spuThName.get_ptr(), spurs->prefix, spurs->prefixSize);
	std::memcpy(spuThName.get_ptr() + spurs->prefixSize, "CellSpursKernel", 16);

	spuThAttr->name                    = spuThName;
	spuThAttr->name_len                = static_cast<u32>(std::strlen(spuThName.get_ptr())) + 2;
	spuThAttr->option                  = SYS_SPU_THREAD_OPTION_DEC_SYNC_TB_ENABLE;
	spuThName[spuThAttr->name_len - 1] = '\0';

	for (s32 num = 0; num < nSpus; num++)
	{
		spuThName[spuThAttr->name_len - 2] = '0' + num;
		spuThArgs->arg1                    = static_cast<u64>(num) << 32;
		spuThArgs->arg2                    = spurs.addr();

		if (s32 rc = sys_spu_thread_initialize(ppu, spurs.ptr(&CellSpurs::spus, num), spurs->spuTG, num, spurs.ptr(&CellSpurs::spuImg), spuThAttr, spuThArgs))
		{
			sys_spu_thread_group_destroy(ppu, spurs->spuTG);
			ppu_execute<&sys_spu_image_close>(ppu, spurs.ptr(&CellSpurs::spuImg));
			return rollback(), rc;
		}

		// entry point cannot be initialized immediately because SPU LS will be rewritten by sys_spu_thread_group_start()
		//idm::get_unlocked<named_thread<spu_thread>>(spurs->spus[num])->custom_task = [entry = spurs->spuImg.entry_point](spu_thread& spu)
		{
			// Disabled
			//spu.RegisterHleFunction(entry, spursKernelEntry);
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

	// Create a mutex to protect access to SPURS handler thread data
	if (vm::var<sys_lwmutex_attribute_t> attr({SYS_SYNC_PRIORITY, SYS_SYNC_NOT_RECURSIVE, {"_spuPrv\0"_u64}});
		s32 rc = ppu_execute<&sys_lwmutex_create>(ppu, lwMutex, +attr))
	{
		_spurs::finalize_spu(ppu, spurs);
		return rollback(), rc;
	}

	// Create condition variable to signal the SPURS handler thread
	if (vm::var<sys_lwcond_attribute_t> attr({"_spuPrv\0"_u64});
		s32 rc = ppu_execute<&sys_lwcond_create>(ppu, lwCond, lwMutex, +attr))
	{
		ppu_execute<&sys_lwmutex_destroy>(ppu, lwMutex);
		_spurs::finalize_spu(ppu, spurs);
		return rollback(), rc;
	}

	spurs->flags1 = (flags & SAF_EXIT_IF_NO_WORK ? SF1_EXIT_IF_NO_WORK : 0) | (isSecond ? SF1_32_WORKLOADS : 0);
	spurs->wklFlagReceiver = 0xff;
	spurs->wklFlag.flag = -1;
	spurs->handlerDirty = 0;
	spurs->handlerWaiting = 0;
	spurs->handlerExiting = 0;
	spurs->ppuPriority = ppuPriority;

	// Create the SPURS event helper thread
	if (s32 rc = _spurs::create_event_helper(ppu, spurs, ppuPriority))
	{
		ppu_execute<&sys_lwcond_destroy>(ppu, lwCond);
		ppu_execute<&sys_lwmutex_destroy>(ppu, lwMutex);
		_spurs::finalize_spu(ppu, spurs);
		return rollback(), rc;
	}

	// Create the SPURS handler thread
	if (s32 rc = _spurs::create_handler(spurs, ppuPriority))
	{
		_spurs::stop_event_helper(ppu, spurs);
		ppu_execute<&sys_lwcond_destroy>(ppu, lwCond);
		ppu_execute<&sys_lwmutex_destroy>(ppu, lwMutex);
		_spurs::finalize_spu(ppu, spurs);
		return rollback(), rc;
	}

	// Enable SPURS exception handler
	if (s32 rc = cellSpursEnableExceptionEventHandler(ppu, spurs, true /*enable*/))
	{
		_spurs::signal_to_handler_thread(ppu, spurs);
		_spurs::join_handler_thread(ppu, spurs);
		_spurs::stop_event_helper(ppu, spurs);
		ppu_execute<&sys_lwcond_destroy>(ppu, lwCond);
		ppu_execute<&sys_lwmutex_destroy>(ppu, lwMutex);
		_spurs::finalize_spu(ppu, spurs);
		return rollback(), rc;
	}

	spurs->traceBuffer = vm::null;
	// TODO: Register libprof for user trace

	// Initialise the event port multiplexor
	_spurs::init_event_port_mux(spurs.ptr(&CellSpurs::eventPortMux), spurs->spuPort, spurs->eventPort, 3);

	// Enable the default system workload if required
	if (flags & SAF_SYSTEM_WORKLOAD_ENABLED)
	{
		ensure(_spurs::add_default_syswkl(spurs, swlPriority, swlMaxSpu, swlIsPreem) == 0);
		return CELL_OK;
	}
	else if (flags & SAF_EXIT_IF_NO_WORK)
	{
		return cellSpursWakeUp(ppu, spurs);
	}

	return CELL_OK;
}

/// Initialize SPURS
s32 cellSpursInitialize(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, s32 nSpus, s32 spuPriority, s32 ppuPriority, b8 exitIfNoWork)
{
	cellSpurs.warning("cellSpursInitialize(spurs=*0x%x, nSpus=%d, spuPriority=%d, ppuPriority=%d, exitIfNoWork=%d)", spurs, nSpus, spuPriority, ppuPriority, exitIfNoWork);

	return _spurs::initialize(ppu, spurs, 0, 0, nSpus, spuPriority, ppuPriority, exitIfNoWork ? SAF_EXIT_IF_NO_WORK : SAF_NONE, vm::null, 0, 0, vm::null, 0, 0);
}

/// Initialise SPURS
s32 cellSpursInitializeWithAttribute(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::cptr<CellSpursAttribute> attr)
{
	cellSpurs.warning("cellSpursInitializeWithAttribute(spurs=*0x%x, attr=*0x%x)", spurs, attr);

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

	return _spurs::initialize(
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
s32 cellSpursInitializeWithAttribute2(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::cptr<CellSpursAttribute> attr)
{
	cellSpurs.warning("cellSpursInitializeWithAttribute2(spurs=*0x%x, attr=*0x%x)", spurs, attr);

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

	return _spurs::initialize(
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
	cellSpurs.warning("_cellSpursAttributeInitialize(attr=*0x%x, revision=%d, sdkVersion=0x%x, nSpus=%d, spuPriority=%d, ppuPriority=%d, exitIfNoWork=%d)",
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
	cellSpurs.warning("cellSpursAttributeSetMemoryContainerForSpuThread(attr=*0x%x, container=0x%x)", attr, container);

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
	cellSpurs.warning("cellSpursAttributeSetNamePrefix(attr=*0x%x, prefix=%s, size=%d)", attr, prefix, size);

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
	cellSpurs.warning("cellSpursAttributeEnableSpuPrintfIfAvailable(attr=*0x%x)", attr);

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
	cellSpurs.warning("cellSpursAttributeSetSpuThreadGroupType(attr=*0x%x, type=%d)", attr, type);

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
	cellSpurs.warning("cellSpursAttributeEnableSystemWorkload(attr=*0x%x, priority=*0x%x, maxSpu=%d, isPreemptible=*0x%x)", attr, priority, maxSpu, isPreemptible);

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
			std::memcpy(attr->swlPriority, priority.get_ptr(), 8);

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
	cellSpurs.todo("cellSpursFinalize(spurs=*0x%x)", spurs);

	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (spurs->handlerExiting)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	[[maybe_unused]] u32 wklEnabled = spurs->wklEnabled.load();

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
	cellSpurs.warning("cellSpursGetSpuThreadGroupId(spurs=*0x%x, group=*0x%x)", spurs, group);

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
	cellSpurs.warning("cellSpursGetNumSpuThread(spurs=*0x%x, nThreads=*0x%x)", spurs, nThreads);

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
	cellSpurs.warning("cellSpursGetSpuThreadId(spurs=*0x%x, thread=*0x%x, nThreads=*0x%x)", spurs, thread, nThreads);

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
	cellSpurs.warning("cellSpursSetMaxContention(spurs=*0x%x, wid=%d, maxContention=%d)", spurs, wid, maxContention);

	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (wid >= spurs->max_workloads())
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	if ((spurs->wklEnabled.load() & (0x80000000u >> wid)) == 0u)
	{
		return CELL_SPURS_CORE_ERROR_SRCH;
	}

	if (spurs->exception)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	if (maxContention > CELL_SPURS_MAX_SPU)
	{
		maxContention = CELL_SPURS_MAX_SPU;
	}

	vm::atomic_op(spurs->wklMaxContention[wid % CELL_SPURS_MAX_WORKLOAD], [&](u8& value)
	{
		value &= wid < CELL_SPURS_MAX_WORKLOAD ? 0xF0 : 0x0F;
		value |= wid < CELL_SPURS_MAX_WORKLOAD ? maxContention : maxContention << 4;
	});

	return CELL_OK;
}

/// Set the priority of a workload on each SPU
s32 cellSpursSetPriorities(vm::ptr<CellSpurs> spurs, u32 wid, vm::cptr<u8[8]> priorities)
{
	cellSpurs.trace("cellSpursSetPriorities(spurs=*0x%x, wid=%d, priorities=*0x%x)", spurs, wid, priorities);

	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (wid >= spurs->max_workloads())
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	if ((spurs->wklEnabled.load() & (0x80000000u >> wid)) == 0u)
	{
		return CELL_SPURS_CORE_ERROR_SRCH;
	}

	if (spurs->exception)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	if (spurs->flags & SAF_SYSTEM_WORKLOAD_ENABLED)
	{
		// TODO: Implement this
	}

	const u64 prio = std::bit_cast<u64>(*priorities);

	// Test if any of the value >= CELL_SPURS_MAX_PRIORITY
	if (prio & 0xf0f0f0f0f0f0f0f0)
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	vm::light_op(spurs->wklInfo(wid).prio64, [&](atomic_t<u64>& v){ v.release(prio); });
	vm::light_op(spurs->sysSrvMsgUpdateWorkload, [](atomic_t<u8>& v){ v.release(0xff); });
	vm::light_op(spurs->sysSrvMessage, [](atomic_t<u8>& v){ v.release(0xff); });
	return CELL_OK;
}

/// Set the priority of a workload for the specified SPU
s32 cellSpursSetPriority(vm::ptr<CellSpurs> spurs, u32 wid, u32 spuId, u32 priority)
{
	cellSpurs.trace("cellSpursSetPriority(spurs=*0x%x, wid=%d, spuId=%d, priority=%d)", spurs, wid, spuId, priority);

	if (!spurs)
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	if (!spurs.aligned())
		return CELL_SPURS_CORE_ERROR_ALIGN;

	if (wid >= spurs->max_workloads())
		return CELL_SPURS_CORE_ERROR_INVAL;

	if (priority >= CELL_SPURS_MAX_PRIORITY || spuId >= spurs->nSpus)
		return CELL_SPURS_CORE_ERROR_INVAL;

	if ((spurs->wklEnabled & (0x80000000u >> wid)) == 0u)
		return CELL_SPURS_CORE_ERROR_SRCH;

	if (spurs->exception)
		return CELL_SPURS_CORE_ERROR_STAT;

	vm::light_op<true>(spurs->wklInfo(wid).priority[spuId], [&](u8& v){ atomic_storage<u8>::release(v, priority); });
	vm::light_op<true>(spurs->sysSrvMsgUpdateWorkload, [&](atomic_t<u8>& v){ v.bit_test_set(spuId); });
	vm::light_op<true>(spurs->sysSrvMessage, [&](atomic_t<u8>& v){ v.bit_test_set(spuId); });
	return CELL_OK;
}

/// Set preemption victim SPU
s32 cellSpursSetPreemptionVictimHints(vm::ptr<CellSpurs> spurs, vm::cptr<b8> isPreemptible)
{
	cellSpurs.todo("cellSpursSetPreemptionVictimHints(spurs=*0x%x, isPreemptible=*0x%x)", spurs, isPreemptible);
	return CELL_OK;
}

/// Attach an LV2 event queue to a SPURS instance
s32 cellSpursAttachLv2EventQueue(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 queue, vm::ptr<u8> port, s32 isDynamic)
{
	cellSpurs.warning("cellSpursAttachLv2EventQueue(spurs=*0x%x, queue=0x%x, port=*0x%x, isDynamic=%d)", spurs, queue, port, isDynamic);

	return _spurs::attach_lv2_eq(ppu, spurs, queue, port, isDynamic, false);
}

/// Detach an LV2 event queue from a SPURS instance
s32 cellSpursDetachLv2EventQueue(vm::ptr<CellSpurs> spurs, u8 port)
{
	cellSpurs.warning("cellSpursDetachLv2EventQueue(spurs=*0x%x, port=%d)", spurs, port);

	return _spurs::detach_lv2_eq(spurs, port, false);
}

s32 cellSpursEnableExceptionEventHandler(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, b8 flag)
{
	cellSpurs.warning("cellSpursEnableExceptionEventHandler(spurs=*0x%x, flag=%d)", spurs, flag);

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
		if (oldEnableEH == 0u)
		{
			rc = sys_spu_thread_group_connect_event(ppu, spurs->spuTG, spurs->eventQueue, SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION);
		}
	}
	else
	{
		if (oldEnableEH == 1u)
		{
			rc = sys_spu_thread_group_disconnect_event(ppu, spurs->eventQueue, SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION);
		}
	}

	return rc;
}

/// Set the global SPU exception event handler
s32 cellSpursSetGlobalExceptionEventHandler(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursGlobalExceptionEventHandler> eaHandler, vm::ptr<void> arg)
{
	cellSpurs.warning("cellSpursSetGlobalExceptionEventHandler(spurs=*0x%x, eaHandler=*0x%x, arg=*0x%x)", spurs, eaHandler, arg);

	if (!spurs || !eaHandler)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (spurs->exception)
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
	cellSpurs.warning("cellSpursUnsetGlobalExceptionEventHandler(spurs=*0x%x)", spurs);

	if (!spurs)
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	if (!spurs.aligned())
		return CELL_SPURS_CORE_ERROR_ALIGN;

	if (spurs->exception)
		return CELL_SPURS_CORE_ERROR_STAT;

	spurs->globalSpuExceptionHandlerArgs = 0;
	spurs->globalSpuExceptionHandler.exchange(0);
	return CELL_OK;
}

/// Get internal information of a SPURS instance
s32 cellSpursGetInfo(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursInfo> info)
{
	cellSpurs.trace("cellSpursGetInfo(spurs=*0x%x, info=*0x%x)", spurs, info);

	if (!spurs || !info)
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	if (!spurs.aligned())
		return CELL_SPURS_CORE_ERROR_ALIGN;

	const auto flags = spurs->flags;
	info->nSpus = spurs->nSpus;
	info->spuThreadGroupPriority = spurs->spuPriority;
	info->ppuThreadPriority = spurs->ppuPriority;
	info->exitIfNoWork = !!(flags & SAF_EXIT_IF_NO_WORK);
	info->spurs2 = !!(flags & SAF_SECOND_VERSION);
	info->spuThreadGroup = spurs->spuTG;
	std::memcpy(&info->spuThreads, &spurs->spus, sizeof(s32) * 8);
	info->spursHandlerThread0 = spurs->ppu0;
	info->spursHandlerThread1 = spurs->ppu1;
	info->traceBufferSize = spurs->traceDataSize;

	const auto trace_addr = vm::cast(spurs->traceBuffer.addr());
	info->traceBuffer = vm::addr_t{trace_addr & ~3};
	info->traceMode = trace_addr & 3;

	const u8 name_size = spurs->prefixSize;
	std::memcpy(&info->namePrefix, spurs->prefix, name_size);
	info->namePrefix[name_size] = '\0';
	info->namePrefixLength = name_size;

	// TODO: Should call sys_spu_thread_group_syscall_253 for specific SPU group types
	info->deadlineMeetCounter = 0;
	info->deadlineMissCounter = 0;
	return CELL_OK;
}

//----------------------------------------------------------------------------
// SPURS SPU GUID functions
//----------------------------------------------------------------------------

/// Get the SPU GUID from a .SpuGUID section
s32 cellSpursGetSpuGuid(vm::cptr<void> guid, vm::ptr<u64> dst)
{
	cellSpurs.trace("cellSpursGetSpuGuid(guid=*0x%x, dst=*0x%x)", guid, dst);

	if (!guid || !dst)
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	if (!dst.aligned())
		return CELL_SPURS_CORE_ERROR_ALIGN;

	return CELL_OK;
}

//----------------------------------------------------------------------------
// SPURS trace functions
//----------------------------------------------------------------------------

void _spurs::trace_status_update(ppu_thread& ppu, vm::ptr<CellSpurs> spurs)
{
	u8 init;

	vm::atomic_op(spurs->sysSrvTrace, [spurs, &init](CellSpurs::SrvTraceSyncVar& data)
	{
		if ((init = data.sysSrvTraceInitialised))
		{
			data.sysSrvNotifyUpdateTraceComplete = 1;
			data.sysSrvMsgUpdateTrace            = (1 << spurs->nSpus) - 1;
		}
	});

	if (init)
	{
		vm::light_op<true>(spurs->sysSrvMessage, [&](atomic_t<u8>& v){ v.release(0xff); });
		ensure(sys_semaphore_wait(ppu, static_cast<u32>(spurs->semPrv), 0) == 0);
		static_cast<void>(ppu.test_stopped());
	}
}

s32 _spurs::trace_initialize(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTraceInfo> buffer, u32 size, u32 mode, u32 updateStatus)
{
	if (!spurs || !buffer)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned() || !buffer.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (size < sizeof(CellSpursTraceInfo) || mode & ~(CELL_SPURS_TRACE_MODE_FLAG_MASK))
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	if (spurs->traceBuffer)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	spurs->traceDataSize = size - u32{sizeof(CellSpursTraceInfo)};
	for (u32 i = 0; i < 8; i++)
	{
		buffer->spuThread[i] = spurs->spus[i];
		buffer->count[i]     = 0;
	}

	buffer->spuThreadGroup = spurs->spuTG;
	buffer->numSpus        = spurs->nSpus;
	spurs->traceBuffer.set(buffer.addr() | (mode & CELL_SPURS_TRACE_MODE_FLAG_WRAP_BUFFER ? 1 : 0));
	spurs->traceMode     = mode;

	u32 spuTraceDataCount = ::narrow<u32>((spurs->traceDataSize / sizeof(CellSpursTracePacket)) / spurs->nSpus);
	for (u32 i = 0, j = 8; i < 6; i++)
	{
		spurs->traceStartIndex[i] = j;
		j += spuTraceDataCount;
	}

	spurs->sysSrvTraceControl = 0;
	if (updateStatus)
	{
		_spurs::trace_status_update(ppu, spurs);
	}

	return CELL_OK;
}

/// Initialize SPURS trace
s32 cellSpursTraceInitialize(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTraceInfo> buffer, u32 size, u32 mode)
{
	cellSpurs.warning("cellSpursTraceInitialize(spurs=*0x%x, buffer=*0x%x, size=0x%x, mode=0x%x)", spurs, buffer, size, mode);

	if (_spurs::is_libprof_loaded())
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	return _spurs::trace_initialize(ppu, spurs, buffer, size, mode, 1);
}

/// Finalize SPURS trace
s32 cellSpursTraceFinalize(ppu_thread& ppu, vm::ptr<CellSpurs> spurs)
{
	cellSpurs.warning("cellSpursTraceFinalize(spurs=*0x%x)", spurs);

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

	_spurs::trace_status_update(ppu, spurs);

	return CELL_OK;
}

s32 _spurs::trace_start(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 updateStatus)
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
		_spurs::trace_status_update(ppu, spurs);
	}

	return CELL_OK;
}

/// Start SPURS trace
s32 cellSpursTraceStart(ppu_thread& ppu, vm::ptr<CellSpurs> spurs)
{
	cellSpurs.warning("cellSpursTraceStart(spurs=*0x%x)", spurs);

	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	return _spurs::trace_start(ppu, spurs, spurs->traceMode & CELL_SPURS_TRACE_MODE_FLAG_SYNCHRONOUS_START_STOP);
}

s32 _spurs::trace_stop(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 updateStatus)
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
		_spurs::trace_status_update(ppu, spurs);
	}

	return CELL_OK;
}

/// Stop SPURS trace
s32 cellSpursTraceStop(ppu_thread& ppu, vm::ptr<CellSpurs> spurs)
{
	cellSpurs.warning("cellSpursTraceStop(spurs=*0x%x)", spurs);

	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	return _spurs::trace_stop(ppu, spurs, spurs->traceMode & CELL_SPURS_TRACE_MODE_FLAG_SYNCHRONOUS_START_STOP);
}

//----------------------------------------------------------------------------
// SPURS policy module functions
//----------------------------------------------------------------------------

/// Initialize attributes of a workload
s32 _cellSpursWorkloadAttributeInitialize(ppu_thread& /*ppu*/, vm::ptr<CellSpursWorkloadAttribute> attr, u32 revision, u32 sdkVersion, vm::cptr<void> pm, u32 size, u64 data, vm::cptr<u8[8]> priority, u32 minCnt, u32 maxCnt)
{
	cellSpurs.warning("_cellSpursWorkloadAttributeInitialize(attr=*0x%x, revision=%d, sdkVersion=0x%x, pm=*0x%x, size=0x%x, data=0x%llx, priority=*0x%x, minCnt=0x%x, maxCnt=0x%x)",
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

	if (!pm.aligned(16))
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	// Load packed priorities (endian-agnostic)
	const u64 prio = std::bit_cast<u64>(*priority);

	// check if some priority > 15
	if (minCnt == 0 || prio & 0xf0f0f0f0f0f0f0f0)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}

	std::memset(attr.get_ptr(), 0, sizeof(CellSpursWorkloadAttribute));
	attr->revision = revision;
	attr->sdkVersion = sdkVersion;
	attr->pm = pm;
	attr->size = size;
	attr->data = data;
	std::memcpy(attr->priority, &prio, 8);
	attr->minContention = minCnt;
	attr->maxContention = maxCnt;
	return CELL_OK;
}

/// Set the name of a workload
s32 cellSpursWorkloadAttributeSetName(ppu_thread& /*ppu*/, vm::ptr<CellSpursWorkloadAttribute> attr, vm::cptr<char> nameClass, vm::cptr<char> nameInstance)
{
	cellSpurs.warning("cellSpursWorkloadAttributeSetName(attr=*0x%x, nameClass=%s, nameInstance=%s)", attr, nameClass, nameInstance);

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
	cellSpurs.warning("cellSpursWorkloadAttributeSetShutdownCompletionEventHook(attr=*0x%x, hook=*0x%x, arg=*0x%x)", attr, hook, arg);

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

s32 _spurs::add_workload(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<u32> wid, vm::cptr<void> pm, u32 size, u64 data, const u8(&priorityTable)[8], u32 minContention, u32 maxContention, vm::cptr<char> nameClass, vm::cptr<char> nameInstance, vm::ptr<CellSpursShutdownCompletionEventHook> hook, vm::ptr<void> hookArg)
{
	if (!spurs || !wid || !pm)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned() || !pm.aligned(16))
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (minContention == 0 || std::bit_cast<u64>(priorityTable) & 0xf0f0f0f0f0f0f0f0ull) // check if some priority > 15
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}

	if (spurs->exception)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	u32 wnum;
	const u32 wmax = spurs->flags1 & SF1_32_WORKLOADS ? CELL_SPURS_MAX_WORKLOAD2 : CELL_SPURS_MAX_WORKLOAD; // TODO: check if can be changed

	vm::fetch_op(spurs->wklEnabled, [&](be_t<u32>& value)
	{
		wnum = std::countl_one<u32>(value); // found empty position

		if (wnum < wmax)
		{
			value |= (0x80000000 >> wnum); // set workload bit
		}
	});

	*wid = wnum; // store workload id
	if (wnum >= wmax)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_AGAIN;
	}

	auto& spurs_res = vm::reservation_acquire(spurs.addr());
	auto& spurs_res2 = vm::reservation_acquire(spurs.addr() + 0x80);

	if (!spurs_res2.fetch_op([&](u64& r)
	{
		if (r & vm::rsrv_unique_lock)
		{
			return false;
		}

		r += 1;
		return true;
	}).second)
	{
		vm::reservation_shared_lock_internal(spurs_res2);
	}

	if (!spurs_res.fetch_op([&](u64& r)
	{
		if (r & vm::rsrv_unique_lock)
		{
			return false;
		}

		r += 1;
		return true;
	}).second)
	{
		vm::reservation_shared_lock_internal(spurs_res);
	}

	u32 index = wnum & 0xf;
	if (wnum <= 15)
	{
		ensure((spurs->wklCurrentContention[wnum] & 0xf) == 0);
		ensure((spurs->wklPendingContention[wnum] & 0xf) == 0);
		spurs->wklState1[wnum].release(SPURS_WKL_STATE_PREPARING);
		spurs->wklStatus1[wnum] = 0;
		spurs->wklEvent1[wnum].release(0);
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
			spurs->wklIdleSpuCountOrReadyCount2[wnum] = 0;
			spurs->wklMinContention[wnum] = minContention > 8 ? 8 : minContention;
		}

		spurs->wklReadyCount1[wnum].release(0);
	}
	else
	{
		ensure((spurs->wklCurrentContention[index] & 0xf0) == 0);
		ensure((spurs->wklPendingContention[index] & 0xf0) == 0);
		spurs->wklState2[index].release(SPURS_WKL_STATE_PREPARING);
		spurs->wklStatus2[index] = 0;
		spurs->wklEvent2[index].release(0);
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

		spurs->wklIdleSpuCountOrReadyCount2[wnum].release(0);
	}

	spurs->wklMaxContention[index].atomic_op([&](u8& v)
	{
		v &= (wnum <= 15 ? 0xf0 : 0x0f);
		v |= (maxContention > 8 ? 8 : maxContention) << (wnum < CELL_SPURS_MAX_WORKLOAD ? 0 : 4);
	});

	(wnum <= 15 ? spurs->wklSignal1 : spurs->wklSignal2).atomic_op([&](be_t<u16>& data)
	{
		data &= ~(0x8000 >> index);
	});

	// Attempt to avoid CAS
	if (spurs->wklFlagReceiver == wnum && spurs->wklFlagReceiver.compare_and_swap(wnum, 0xff))
	{
		//
	}

	spurs_res += 127;
	spurs_res2 += 127;

	spurs_res.notify_all();
	spurs_res2.notify_all();

	u32 res_wkl;
	const auto wkl = &spurs->wklInfo(wnum);
	vm::reservation_op(ppu, vm::unsafe_ptr_cast<spurs_wkl_state_op>(spurs.ptr(&CellSpurs::wklState1)), [&](spurs_wkl_state_op& op)
	{
		const u32 mask = op.wklMskB & ~(0x80000000u >> wnum);
		res_wkl = 0;

		for (u32 i = 0, m = 0x80000000, k = 0; i < 32; i++, m >>= 1)
		{
			if (mask & m)
			{
				const auto current = &spurs->wklInfo(i);
				if (current->addr == wkl->addr)
				{
					// if a workload with identical policy module found
					res_wkl = current->uniqueId;
					break;
				}
				else
				{
					k |= 0x80000000 >> current->uniqueId;
					res_wkl = std::countl_one<u32>(k);
				}
			}
		}

		wkl->uniqueId.release(static_cast<u8>(res_wkl));
		op.wklMskB = mask | (0x80000000u >> wnum);
		(wnum < CELL_SPURS_MAX_WORKLOAD ? op.wklState1[wnum] : op.wklState2[wnum % 16]) = SPURS_WKL_STATE_RUNNABLE;
	});

	ensure((res_wkl <= 31));
	vm::light_op<true>(spurs->sysSrvMsgUpdateWorkload, [](atomic_t<u8>& v){ v.release(0xff); });
	vm::light_op<true>(spurs->sysSrvMessage, [](atomic_t<u8>& v){ v.release(0xff); });
	return CELL_OK;
}

/// Add workload
s32 cellSpursAddWorkload(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<u32> wid, vm::cptr<void> pm, u32 size, u64 data, vm::cptr<u8[8]> priority, u32 minCnt, u32 maxCnt)
{
	cellSpurs.trace("cellSpursAddWorkload(spurs=*0x%x, wid=*0x%x, pm=*0x%x, size=0x%x, data=0x%llx, priority=*0x%x, minCnt=0x%x, maxCnt=0x%x)",
		spurs, wid, pm, size, data, priority, minCnt, maxCnt);

	return _spurs::add_workload(ppu, spurs, wid, pm, size, data, *priority, minCnt, maxCnt, vm::null, vm::null, vm::null, vm::null);
}

/// Add workload
s32 cellSpursAddWorkloadWithAttribute(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<u32> wid, vm::cptr<CellSpursWorkloadAttribute> attr)
{
	cellSpurs.trace("cellSpursAddWorkloadWithAttribute(spurs=*0x%x, wid=*0x%x, attr=*0x%x)", spurs, wid, attr);

	if (!attr)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (attr->revision != 1u)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}

	return _spurs::add_workload(ppu, spurs, wid, attr->pm, attr->size, attr->data, attr->priority, attr->minContention, attr->maxContention, attr->nameClass, attr->nameInstance, attr->hook, attr->hookArg);
}

/// Request workload shutdown
s32 cellSpursShutdownWorkload(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 wid)
{
	cellSpurs.trace("cellSpursShutdownWorkload(spurs=*0x%x, wid=0x%x)", spurs, wid);

	if (!spurs)
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;

	if (!spurs.aligned())
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;

	if (wid >= spurs->max_workloads())
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;

	if (spurs->exception)
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;

	bool send_event;
	s32 rc, old_state;
	if (!vm::reservation_op(ppu, vm::unsafe_ptr_cast<spurs_wkl_state_op>(spurs.ptr(&CellSpurs::wklState1)), [&](spurs_wkl_state_op& op)
	{
		auto& state = wid < CELL_SPURS_MAX_WORKLOAD ? op.wklState1[wid] : op.wklState2[wid % 16];

		if (state <= SPURS_WKL_STATE_PREPARING)
		{
			rc = CELL_SPURS_POLICY_MODULE_ERROR_STAT;
			return false;
		}

		if (state == SPURS_WKL_STATE_SHUTTING_DOWN || state == SPURS_WKL_STATE_REMOVABLE)
		{
			rc = CELL_OK;
			return false;
		}

		auto& status = wid < CELL_SPURS_MAX_WORKLOAD ? op.wklStatus1[wid] : op.wklStatus2[wid % 16];

		old_state = state = status ? SPURS_WKL_STATE_SHUTTING_DOWN : SPURS_WKL_STATE_REMOVABLE;

		if (state == SPURS_WKL_STATE_SHUTTING_DOWN)
		{
			op.sysSrvMsgUpdateWorkload = -1;
			rc = CELL_OK;
			return true;
		}

		auto& event = wid < CELL_SPURS_MAX_WORKLOAD ? op.wklEvent1[wid] : op.wklEvent2[wid % 16];
		send_event = event & 0x12 && !(event & 1);
		event |= 1;
		rc = CELL_OK;
		return true;
	}))
	{
		return rc;
	}

	if (old_state == SPURS_WKL_STATE_SHUTTING_DOWN)
	{
		vm::light_op<true>(spurs->sysSrvMessage, [&](atomic_t<u8>& v){ v.release(0xff); });
		return CELL_OK;
	}

	if (send_event && sys_event_port_send(spurs->eventPort, 0, 0, (1u << 31) >> wid))
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	return CELL_OK;
}

/// Wait for workload shutdown
s32 cellSpursWaitForWorkloadShutdown(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 wid)
{
	cellSpurs.trace("cellSpursWaitForWorkloadShutdown(spurs=*0x%x, wid=0x%x)", spurs, wid);

	if (!spurs)
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;

	if (!spurs.aligned())
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;

	if (wid >= spurs->max_workloads())
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;

	if (!(spurs->wklEnabled & (0x80000000u >> wid)))
		return CELL_SPURS_POLICY_MODULE_ERROR_SRCH;

	if (spurs->exception)
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;

	auto& info = spurs->wklSyncInfo(wid);

	const auto [_old0, ok] = vm::fetch_op(info.x28, [](be_t<u32>& val)
	{
		if (val)
		{
			return false;
		}

		val = 2;
		return true;
	});

	if (!ok)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	const auto [_old1, wait_sema] = vm::fetch_op<true>(spurs->wklEvent(wid), [](u8& event)
	{
		if ((event & 1) == 0 || (event & 0x22) == 0x2)
		{
			event |= 0x10;
			return true;
		}

		return false;
	});

	if (wait_sema)
	{
		ensure(sys_semaphore_wait(ppu, static_cast<u32>(info.sem), 0) == 0);
	}

	// Reverified
	if (spurs->exception)
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;

	return CELL_OK;
}

s32 cellSpursRemoveSystemWorkloadForUtility()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

/// Remove workload
s32 cellSpursRemoveWorkload(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 wid)
{
	cellSpurs.trace("cellSpursRemoveWorkload(spurs=*0x%x, wid=%u)", spurs, wid);

	if (!spurs)
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;

	if (!spurs.aligned())
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;

	if (wid >= CELL_SPURS_MAX_WORKLOAD2 || (wid >= CELL_SPURS_MAX_WORKLOAD && (spurs->flags1 & SF1_32_WORKLOADS) == 0))
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;

	if (!(spurs->wklEnabled.load() & (0x80000000u >> wid)))
		return CELL_SPURS_POLICY_MODULE_ERROR_SRCH;

	if (spurs->exception)
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;

	switch (spurs->wklState(wid))
	{
	case SPURS_WKL_STATE_SHUTTING_DOWN: return CELL_SPURS_POLICY_MODULE_ERROR_BUSY;
	case SPURS_WKL_STATE_REMOVABLE: break;
	default: return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	if (spurs->wklFlagReceiver == wid)
	{
		ensure(ppu_execute<&_cellSpursWorkloadFlagReceiver>(ppu, spurs, wid, 0) == 0);
	}

	s32 rc;
	vm::reservation_op(ppu, vm::unsafe_ptr_cast<spurs_wkl_state_op>(spurs.ptr(&CellSpurs::wklState1)), [&](spurs_wkl_state_op& op)
	{
		auto& state = wid < CELL_SPURS_MAX_WORKLOAD ? op.wklState1[wid] : op.wklState2[wid % 16];

		// Re-verification, does not exist on realfw
		switch (state)
		{
		case SPURS_WKL_STATE_SHUTTING_DOWN: rc = CELL_SPURS_POLICY_MODULE_ERROR_BUSY; return false;
		case SPURS_WKL_STATE_REMOVABLE: break;
		default: rc = CELL_SPURS_POLICY_MODULE_ERROR_STAT; return false;
		}

		state = SPURS_WKL_STATE_NON_EXISTENT;

		op.wklEnabled &= ~(0x80000000u >> wid);
		op.wklMskB &= ~(0x80000000u >> wid);
		rc = CELL_OK;
		return true;
	});

	return rc;
}

s32 cellSpursWakeUp(ppu_thread& ppu, vm::ptr<CellSpurs> spurs)
{
	cellSpurs.warning("cellSpursWakeUp(spurs=*0x%x)", spurs);

	if (!spurs)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (spurs->exception)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	spurs->handlerDirty.exchange(1);

	if (spurs->handlerWaiting)
	{
		_spurs::signal_to_handler_thread(ppu, spurs);
	}

	return CELL_OK;
}

/// Send a workload signal
s32 cellSpursSendWorkloadSignal(ppu_thread& /*ppu*/, vm::ptr<CellSpurs> spurs, u32 wid)
{
	cellSpurs.warning("cellSpursSendWorkloadSignal(spurs=*0x%x, wid=%d)", spurs, wid);

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

	if (!(spurs->wklEnabled.load() & (0x80000000u >> wid)))
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_SRCH;
	}

	if (spurs->exception)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	if (spurs->wklState(wid) != SPURS_WKL_STATE_RUNNABLE)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	vm::light_op<true>(wid < CELL_SPURS_MAX_WORKLOAD ? spurs->wklSignal1 : spurs->wklSignal2, [&](atomic_be_t<u16>& sig)
	{
		sig |= 0x8000 >> (wid % 16);
	});

	return CELL_OK;
}

/// Get the address of the workload flag
s32 cellSpursGetWorkloadFlag(vm::ptr<CellSpurs> spurs, vm::pptr<CellSpursWorkloadFlag> flag)
{
	cellSpurs.warning("cellSpursGetWorkloadFlag(spurs=*0x%x, flag=**0x%x)", spurs, flag);

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
s32 cellSpursReadyCountStore(ppu_thread& /*ppu*/, vm::ptr<CellSpurs> spurs, u32 wid, u32 value)
{
	cellSpurs.trace("cellSpursReadyCountStore(spurs=*0x%x, wid=%d, value=0x%x)", spurs, wid, value);

	if (!spurs)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (wid >= spurs->max_workloads() || value > 0xffu)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}

	if ((spurs->wklEnabled.load() & (0x80000000u >> wid)) == 0u)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_SRCH;
	}

	if (spurs->exception || spurs->wklState(wid) != SPURS_WKL_STATE_RUNNABLE)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	vm::light_op<true>(spurs->readyCount(wid), [&](atomic_t<u8>& v)
	{
		v.release(static_cast<u8>(value));
	});

	return CELL_OK;
}

/// Swap ready count
s32 cellSpursReadyCountSwap(ppu_thread& /*ppu*/, vm::ptr<CellSpurs> spurs, u32 wid, vm::ptr<u32> old, u32 swap)
{
	cellSpurs.trace("cellSpursReadyCountSwap(spurs=*0x%x, wid=%d, old=*0x%x, swap=0x%x)", spurs, wid, old, swap);

	if (!spurs || !old)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (wid >= spurs->max_workloads() || swap > 0xffu)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}

	if ((spurs->wklEnabled.load() & (0x80000000u >> wid)) == 0u)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_SRCH;
	}

	if (spurs->exception || spurs->wklState(wid) != SPURS_WKL_STATE_RUNNABLE)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	*old = vm::light_op(spurs->readyCount(wid), [&](atomic_t<u8>& v)
	{
		return v.exchange(static_cast<u8>(swap));
	});

	return CELL_OK;
}

/// Compare and swap ready count
s32 cellSpursReadyCountCompareAndSwap(ppu_thread& /*ppu*/, vm::ptr<CellSpurs> spurs, u32 wid, vm::ptr<u32> old, u32 compare, u32 swap)
{
	cellSpurs.trace("cellSpursReadyCountCompareAndSwap(spurs=*0x%x, wid=%d, old=*0x%x, compare=0x%x, swap=0x%x)", spurs, wid, old, compare, swap);

	if (!spurs || !old)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (wid >= spurs->max_workloads() || (swap | compare) > 0xffu)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}

	if ((spurs->wklEnabled.load() & (0x80000000u >> wid)) == 0u)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_SRCH;
	}

	if (spurs->exception || spurs->wklState(wid) != SPURS_WKL_STATE_RUNNABLE)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	u8 temp = static_cast<u8>(compare);

	vm::light_op(spurs->readyCount(wid), [&](atomic_t<u8>& v)
	{
		v.compare_exchange(temp, static_cast<u8>(swap));
	});

	*old = temp;
	return CELL_OK;
}

/// Increase or decrease ready count
s32 cellSpursReadyCountAdd(ppu_thread& /*ppu*/, vm::ptr<CellSpurs> spurs, u32 wid, vm::ptr<u32> old, s32 value)
{
	cellSpurs.trace("cellSpursReadyCountAdd(spurs=*0x%x, wid=%d, old=*0x%x, value=0x%x)", spurs, wid, old, value);

	if (!spurs || !old)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (wid >= spurs->max_workloads())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}

	if ((spurs->wklEnabled.load() & (0x80000000u >> wid)) == 0u)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_SRCH;
	}

	if (spurs->exception || spurs->wklState(wid) != SPURS_WKL_STATE_RUNNABLE)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	*old = vm::fetch_op(spurs->readyCount(wid), [&](u8& val)
	{
		val = static_cast<u8>(std::clamp<s32>(val + static_cast<u32>(value), 0, 255));
	});

	return CELL_OK;
}

/// Get workload's data to be passed to policy module
s32 cellSpursGetWorkloadData(vm::ptr<CellSpurs> spurs, vm::ptr<u64> data, u32 wid)
{
	cellSpurs.trace("cellSpursGetWorkloadData(spurs=*0x%x, data=*0x%x, wid=%d)", spurs, data, wid);

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

	if ((spurs->wklEnabled.load() & (0x80000000u >> wid)) == 0u)
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
s32 cellSpursGetWorkloadInfo(ppu_thread& /*ppu*/, vm::ptr<CellSpurs> spurs, u32 wid, vm::ptr<CellSpursWorkloadInfo> info)
{
	cellSpurs.todo("cellSpursGetWorkloadInfo(spurs=*0x%x, wid=0x%x, info=*0x%x)", spurs, wid, info);
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
s32 _cellSpursWorkloadFlagReceiver(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, u32 wid, u32 is_set)
{
	cellSpurs.warning("_cellSpursWorkloadFlagReceiver(spurs=*0x%x, wid=%d, is_set=%d)", spurs, wid, is_set);

	if (!spurs)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (wid >= spurs->max_workloads())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}

	if ((spurs->wklEnabled.load() & (0x80000000u >> wid)) == 0u)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_SRCH;
	}

	if (spurs->exception)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	atomic_fence_acq_rel();

	struct alignas(128) wklFlagOp
	{
		u8 uns[0x6C];
		be_t<u32> Flag; // 0x6C
		u8 uns2[0x7];
		u8 FlagReceiver; // 0x77
	};

	s32 res = CELL_OK;

	vm::reservation_op(ppu, vm::unsafe_ptr_cast<wklFlagOp>(spurs), [&](wklFlagOp& val)
	{
		if (is_set)
		{
			if (val.FlagReceiver != 0xff)
			{
				res = CELL_SPURS_POLICY_MODULE_ERROR_BUSY;
				return;
			}
		}
		else
		{
			if (val.FlagReceiver != wid)
			{
				res = CELL_SPURS_POLICY_MODULE_ERROR_PERM;
				return;
			}
		}

		val.Flag = -1;

		if (is_set)
		{
			if (val.FlagReceiver == 0xff)
			{
				val.FlagReceiver = static_cast<u8>(wid);
			}
		}
		else
		{
			if (val.FlagReceiver == wid)
			{
				val.FlagReceiver = 0xff;
			}
		}

		res = CELL_OK;
	});

	return res;
}

/// Set/unset the recipient of the workload flag
s32 _cellSpursWorkloadFlagReceiver2(ppu_thread& /*ppu*/, vm::ptr<CellSpurs> spurs, u32 wid, u32 is_set, u32 print_debug_output)
{
	cellSpurs.warning("_cellSpursWorkloadFlagReceiver2(spurs=*0x%x, wid=%d, is_set=%d, print_debug_output=%d)", spurs, wid, is_set, print_debug_output);

	if (!spurs)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (wid >= spurs->max_workloads())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}

	if ((spurs->wklEnabled.load() & (0x80000000u >> wid)) == 0u)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_SRCH;
	}

	if (spurs->exception)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	atomic_fence_acq_rel();

	s32 res = CELL_OK;

	vm::atomic_op<true>(spurs->wklFlagReceiver, [&](u8& FlagReceiver)
	{
		if (is_set)
		{
			if (FlagReceiver != 0xff)
			{
				res = CELL_SPURS_POLICY_MODULE_ERROR_BUSY;
				return;
			}
		}
		else
		{
			if (FlagReceiver != wid)
			{
				res = CELL_SPURS_POLICY_MODULE_ERROR_PERM;
				return;
			}
		}

		if (is_set)
		{
			if (FlagReceiver == 0xff)
			{
				FlagReceiver = static_cast<u8>(wid);
			}
		}
		else
		{
			if (FlagReceiver == wid)
			{
				FlagReceiver = 0xff;
			}
		}

		res = CELL_OK;
	});

	return res;
}

/// Request assignment of idle SPUs
s32 cellSpursRequestIdleSpu(vm::ptr<CellSpurs> spurs, u32 wid, u32 count)
{
	cellSpurs.trace("cellSpursRequestIdleSpu(spurs=*0x%x, wid=%d, count=%d)", spurs, wid, count);

	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (!spurs.aligned())
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	// Old API: This function doesn't support 32 workloads
	if (spurs->flags1 & SF1_32_WORKLOADS)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	if (wid >= CELL_SPURS_MAX_WORKLOAD || count >= CELL_SPURS_MAX_SPU)
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	if ((spurs->wklEnabled.load() & (0x80000000u >> wid)) == 0u)
	{
		return CELL_SPURS_CORE_ERROR_SRCH;
	}

	if (spurs->exception)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	vm::light_op<true>(spurs->wklIdleSpuCountOrReadyCount2[wid], FN(x.release(static_cast<u8>(count))));
	return CELL_OK;
}

//----------------------------------------------------------------------------
// SPURS event flag functions
//----------------------------------------------------------------------------

/// Initialize a SPURS event flag
s32 _cellSpursEventFlagInitialize(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursEventFlag> eventFlag, u32 flagClearMode, u32 flagDirection)
{
	cellSpurs.warning("_cellSpursEventFlagInitialize(spurs=*0x%x, taskset=*0x%x, eventFlag=*0x%x, flagClearMode=%d, flagDirection=%d)", spurs, taskset, eventFlag, flagClearMode, flagDirection);

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
	cellSpurs.warning("cellSpursEventFlagClear(eventFlag=*0x%x, bits=0x%x)", eventFlag, bits);

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
s32 cellSpursEventFlagSet(ppu_thread& ppu, vm::ptr<CellSpursEventFlag> eventFlag, u16 bits)
{
	cellSpurs.trace("cellSpursEventFlagSet(eventFlag=*0x%x, bits=0x%x)", eventFlag, bits);

	if (!eventFlag)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!eventFlag.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (auto dir = eventFlag->direction; dir != CELL_SPURS_EVENT_FLAG_SPU2PPU && dir != CELL_SPURS_EVENT_FLAG_ANY2ANY)
	{
		return CELL_SPURS_TASK_ERROR_PERM;
	}

	bool send;
	u8   ppuWaitSlot;
	u16  ppuEvents;
	u16  pendingRecv;
	u16  pendingRecvTaskEvents[16];

	vm::reservation_op(ppu, vm::unsafe_ptr_cast<CellSpursEventFlag_x00>(eventFlag), [bits, &send, &ppuWaitSlot, &ppuEvents, &pendingRecv, &pendingRecvTaskEvents](CellSpursEventFlag_x00& eventFlag)
	{
		send        = false;
		ppuWaitSlot = 0;
		ppuEvents   = 0;
		pendingRecv = 0;

		u16 eventsToClear = 0;

		auto& ctrl = eventFlag.ctrl;
		if (eventFlag.direction == CELL_SPURS_EVENT_FLAG_ANY2ANY && ctrl.ppuWaitMask)
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
		u16 relevantWaitSlots = eventFlag.spuTaskUsedWaitSlots & ~ctrl.spuTaskPendingRecv;
		while (relevantWaitSlots)
		{
			if (relevantWaitSlots & 0x0001)
			{
				u16 spuTaskRelevantEvents = (ctrl.events | bits) & eventFlag.spuTaskWaitMask[i];

				// Unblock the waiting SPU task if either all the bits being waited by the task have been set or
				// if the wait mode of the task is OR and atleast one bit the thread is waiting on has been set
				if ((eventFlag.spuTaskWaitMask[i] & ~spuTaskRelevantEvents) == 0 ||
					(((eventFlag.spuTaskWaitMode >> j) & 0x0001) == CELL_SPURS_EVENT_FLAG_OR && spuTaskRelevantEvents != 0))
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
		if (eventFlag.clearMode == CELL_SPURS_EVENT_FLAG_CLEAR_AUTO)
		{
			 ctrl.events &= ~eventsToClear;
		}

		//eventFlagControl = ((u64)events << 48) | ((u64)spuTaskPendingRecv << 32) | ((u64)ppuWaitMask << 16) | ((u64)ppuWaitSlotAndMode << 8) | (u64)ppuPendingRecv;
	});

	if (send)
	{
		// Signal the PPU thread to be woken up
		eventFlag->pendingRecvTaskEvents[ppuWaitSlot] = ppuEvents;

		ensure(sys_event_port_send(eventFlag->eventPortId, 0, 0, 0) == 0);
		static_cast<void>(ppu.test_stopped());
	}

	if (pendingRecv)
	{
		// Signal each SPU task whose conditions have been met to be woken up
		for (s32 i = 0; i < CELL_SPURS_EVENT_FLAG_MAX_WAIT_SLOTS; i++)
		{
			if (pendingRecv & (0x8000 >> i))
			{
				eventFlag->pendingRecvTaskEvents[i] = pendingRecvTaskEvents[i];
				vm::var<vm::ptr<CellSpursTaskset>> taskset;
				if (eventFlag->isIwl)
				{
					cellSpursLookUpTasksetAddress(ppu, vm::cast(eventFlag->addr), taskset, eventFlag->waitingTaskWklId[i]);
				}
				else
				{
					*taskset = vm::cast(eventFlag->addr);
				}

				auto rc = _cellSpursSendSignal(ppu, *taskset, eventFlag->waitingTaskId[i]);
				if (rc + 0u == CELL_SPURS_TASK_ERROR_INVAL || rc + 0u == CELL_SPURS_TASK_ERROR_STAT)
				{
					return CELL_SPURS_TASK_ERROR_FATAL;
				}

				ensure(rc == CELL_OK);
			}
		}
	}

	return CELL_OK;
}

s32 _spurs::event_flag_wait(ppu_thread& ppu, vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u16> mask, u32 mode, u32 block)
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

	if (auto dir = eventFlag->direction; dir != CELL_SPURS_EVENT_FLAG_SPU2PPU && dir != CELL_SPURS_EVENT_FLAG_ANY2ANY)
	{
		return CELL_SPURS_TASK_ERROR_PERM;
	}

	if (block && eventFlag->spuPort == CELL_SPURS_EVENT_FLAG_INVALID_SPU_PORT)
	{
		return CELL_SPURS_TASK_ERROR_STAT;
	}

	if (eventFlag->ctrl.raw().ppuWaitMask || eventFlag->ctrl.raw().ppuPendingRecv)
	{
		return CELL_SPURS_TASK_ERROR_BUSY;
	}

	bool recv;
	s32  rc;
	u16  receivedEvents;
	vm::atomic_op(eventFlag->ctrl, [eventFlag, mask, mode, block, &recv, &rc, &receivedEvents](CellSpursEventFlag::ControlSyncVar& ctrl)
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
		ensure(sys_event_queue_receive(ppu, eventFlag->eventQueueId, vm::null, 0) == 0);
		static_cast<void>(ppu.test_stopped());

		s32 i = 0;
		if (eventFlag->direction == CELL_SPURS_EVENT_FLAG_ANY2ANY)
		{
			i = eventFlag->ctrl.raw().ppuWaitSlotAndMode >> 4;
		}

		*mask = eventFlag->pendingRecvTaskEvents[i];
		vm::atomic_op(eventFlag->ctrl, [](CellSpursEventFlag::ControlSyncVar& ctrl) { ctrl.ppuPendingRecv = 0; });
	}

	*mask = receivedEvents;
	return CELL_OK;
}

/// Wait for SPURS event flag
s32 cellSpursEventFlagWait(ppu_thread& ppu, vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u16> mask, u32 mode)
{
	cellSpurs.warning("cellSpursEventFlagWait(eventFlag=*0x%x, mask=*0x%x, mode=%d)", eventFlag, mask, mode);

	return _spurs::event_flag_wait(ppu, eventFlag, mask, mode, 1);
}

/// Check SPURS event flag
s32 cellSpursEventFlagTryWait(ppu_thread& ppu, vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u16> mask, u32 mode)
{
	cellSpurs.warning("cellSpursEventFlagTryWait(eventFlag=*0x%x, mask=*0x%x, mode=0x%x)", eventFlag, mask, mode);

	return _spurs::event_flag_wait(ppu, eventFlag, mask, mode, 0);
}

/// Attach an LV2 event queue to a SPURS event flag
s32 cellSpursEventFlagAttachLv2EventQueue(ppu_thread& ppu, vm::ptr<CellSpursEventFlag> eventFlag)
{
	cellSpurs.warning("cellSpursEventFlagAttachLv2EventQueue(eventFlag=*0x%x)", eventFlag);

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
		spurs = vm::cast(eventFlag->addr);
	}
	else
	{
		auto taskset = vm::ptr<CellSpursTaskset>::make(vm::cast(eventFlag->addr));
		spurs = taskset->spurs;
	}

	vm::var<u32> eventQueueId;
	vm::var<u8>  port;

	auto failure = [](s32 rc) -> s32
	{
		// Return rc if its an error code from SPURS otherwise convert the error code to a SPURS task error code
		return (rc & 0x0FFF0000) == 0x00410000 ? rc : (0x80410900 | (rc & 0xFF));
	};

	if (s32 rc = _spurs::create_lv2_eq(ppu, spurs, eventQueueId, port, 1, sys_event_queue_attribute_t{SYS_SYNC_PRIORITY, SYS_PPU_QUEUE, {"_spuEvF\0"_u64}}))
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
		vm::var<u32> eventPortId;

		s32 rc = sys_event_port_create(ppu, eventPortId, SYS_EVENT_PORT_LOCAL, 0);
		if (rc == CELL_OK)
		{
			rc = sys_event_port_connect_local(ppu, *eventPortId, *eventQueueId);
			if (rc == CELL_OK)
			{
				eventFlag->eventPortId = *eventPortId;
				return success(), CELL_OK;
			}

			sys_event_port_destroy(ppu, *eventPortId);
		}

		if (_spurs::detach_lv2_eq(spurs, *port, true) == CELL_OK)
		{
			sys_event_queue_destroy(ppu, *eventQueueId, SYS_EVENT_QUEUE_DESTROY_FORCE);
		}

		return failure(rc);
	}

	return success(), CELL_OK;
}

/// Detach an LV2 event queue from SPURS event flag
s32 cellSpursEventFlagDetachLv2EventQueue(ppu_thread& ppu, vm::ptr<CellSpursEventFlag> eventFlag)
{
	cellSpurs.warning("cellSpursEventFlagDetachLv2EventQueue(eventFlag=*0x%x)", eventFlag);

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

	if (eventFlag->ctrl.raw().ppuWaitMask || eventFlag->ctrl.raw().ppuPendingRecv)
	{
		return CELL_SPURS_TASK_ERROR_BUSY;
	}

	const u8 port = eventFlag->spuPort;

	eventFlag->spuPort = CELL_SPURS_EVENT_FLAG_INVALID_SPU_PORT;

	vm::ptr<CellSpurs> spurs;
	if (eventFlag->isIwl == 1)
	{
		spurs = vm::cast(eventFlag->addr);
	}
	else
	{
		auto taskset = vm::ptr<CellSpursTaskset>::make(vm::cast(eventFlag->addr));
		spurs = taskset->spurs;
	}

	if (eventFlag->direction == CELL_SPURS_EVENT_FLAG_ANY2ANY)
	{
		sys_event_port_disconnect(ppu, eventFlag->eventPortId);
		sys_event_port_destroy(ppu, eventFlag->eventPortId);
	}

	s32 rc = _spurs::detach_lv2_eq(spurs, port, true);

	if (rc == CELL_OK)
	{
		rc = sys_event_queue_destroy(ppu, eventFlag->eventQueueId, SYS_EVENT_QUEUE_DESTROY_FORCE);
	}

	return CELL_OK;
}

/// Get send-receive direction of the SPURS event flag
s32 cellSpursEventFlagGetDirection(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u32> direction)
{
	cellSpurs.warning("cellSpursEventFlagGetDirection(eventFlag=*0x%x, direction=*0x%x)", eventFlag, direction);

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
	cellSpurs.warning("cellSpursEventFlagGetClearMode(eventFlag=*0x%x, clear_mode=*0x%x)", eventFlag, clear_mode);

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
	cellSpurs.warning("cellSpursEventFlagGetTasksetAddress(eventFlag=*0x%x, taskset=**0x%x)", eventFlag, taskset);

	if (!eventFlag || !taskset)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!eventFlag.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	taskset->set(eventFlag->isIwl ? 0u : vm::cast(eventFlag->addr));
	return CELL_OK;
}

static inline s32 SyncErrorToSpursError(s32 res)
{
	return res < 0 ? 0x80410900 | (res & 0xff) : res;
}

s32 _cellSpursLFQueueInitialize(vm::ptr<void> pTasksetOrSpurs, vm::ptr<CellSpursLFQueue> pQueue, vm::cptr<void> buffer, u32 size, u32 depth, u32 direction)
{
	cellSpurs.todo("_cellSpursLFQueueInitialize(pTasksetOrSpurs=*0x%x, pQueue=*0x%x, buffer=*0x%x, size=0x%x, depth=0x%x, direction=%d)", pTasksetOrSpurs, pQueue, buffer, size, depth, direction);

	return SyncErrorToSpursError(cellSyncLFQueueInitialize(pQueue, buffer, size, depth, direction, pTasksetOrSpurs));
}

s32 _cellSpursLFQueuePushBody()
{
	cellSpurs.todo("_cellSpursLFQueuePushBody()");
	return CELL_OK;
}

s32 cellSpursLFQueueAttachLv2EventQueue(vm::ptr<CellSyncLFQueue> queue)
{
	cellSpurs.todo("cellSpursLFQueueAttachLv2EventQueue(queue=*0x%x)", queue);
	return CELL_OK;
}

s32 cellSpursLFQueueDetachLv2EventQueue(vm::ptr<CellSyncLFQueue> queue)
{
	cellSpurs.todo("cellSpursLFQueueDetachLv2EventQueue(queue=*0x%x)", queue);
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

s32 _spurs::create_taskset(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, u64 args, vm::cptr<u8[8]> priority, u32 max_contention, vm::cptr<char> name, u32 size, s32 enable_clear_ls)
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

	vm::var<CellSpursWorkloadAttribute> wkl_attr;
	_cellSpursWorkloadAttributeInitialize(ppu, wkl_attr, 1, SYS_PROCESS_PARAM_VERSION_330_0, vm::cptr<void>::make(SPURS_IMG_ADDR_TASKSET_PM), 0x1E40 /*pm_size*/,
		taskset.addr(), priority, 8, max_contention);
	// TODO: Check return code

	cellSpursWorkloadAttributeSetName(ppu, wkl_attr, vm::null, name);
	// TODO: Check return code

	// TODO: cellSpursWorkloadAttributeSetShutdownCompletionEventHook(wkl_attr, hook, taskset);
	// TODO: Check return code

	vm::var<u32> wid;
	cellSpursAddWorkloadWithAttribute(ppu, spurs, wid, wkl_attr);
	// TODO: Check return code

	taskset->wkl_flag_wait_task = 0x80;
	taskset->wid                = *wid;
	// TODO: cellSpursSetExceptionEventHandler(spurs, wid, hook, taskset);
	// TODO: Check return code

	return CELL_OK;
}

s32 cellSpursCreateTasksetWithAttribute(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursTasksetAttribute> attr)
{
	cellSpurs.warning("cellSpursCreateTasksetWithAttribute(spurs=*0x%x, taskset=*0x%x, attr=*0x%x)", spurs, taskset, attr);

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

	auto rc = _spurs::create_taskset(ppu, spurs, taskset, attr->args, attr.ptr(&CellSpursTasksetAttribute::priority), attr->max_contention, attr->name, attr->taskset_size, attr->enable_clear_ls);

	if (attr->taskset_size >= sizeof(CellSpursTaskset2))
	{
		// TODO: Implement this
	}

	return rc;
}

s32 cellSpursCreateTaskset(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, u64 args, vm::cptr<u8[8]> priority, u32 maxContention)
{
	cellSpurs.warning("cellSpursCreateTaskset(spurs=*0x%x, taskset=*0x%x, args=0x%llx, priority=*0x%x, maxContention=%d)", spurs, taskset, args, priority, maxContention);

	return _spurs::create_taskset(ppu, spurs, taskset, args, priority, maxContention, vm::null, sizeof(CellSpursTaskset), 0);
}

s32 cellSpursJoinTaskset(vm::ptr<CellSpursTaskset> taskset)
{
	cellSpurs.warning("cellSpursJoinTaskset(taskset=*0x%x)", taskset);

	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursGetTasksetId(vm::ptr<CellSpursTaskset> taskset, vm::ptr<u32> wid)
{
	cellSpurs.warning("cellSpursGetTasksetId(taskset=*0x%x, wid=*0x%x)", taskset, wid);

	if (!taskset || !wid)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!taskset.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	// Does not check its validity
	*wid = taskset->wid;
	return CELL_OK;
}

s32 cellSpursShutdownTaskset(ppu_thread& ppu, vm::ptr<CellSpursTaskset> taskset)
{
	cellSpurs.warning("cellSpursShutdownTaskset(taskset=*0x%x)", taskset);

	if (!taskset)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!taskset.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	const u32 wid = taskset->wid;

	if (wid >= CELL_SPURS_MAX_WORKLOAD)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	const auto spurs = +taskset->spurs;
	u32 shutdown_error = ppu_execute<&cellSpursShutdownWorkload>(ppu, spurs, wid);

	if (shutdown_error == CELL_SPURS_POLICY_MODULE_ERROR_STAT)
	{
		shutdown_error = CELL_SPURS_TASK_ERROR_STAT;
	}
	else if (shutdown_error == CELL_SPURS_POLICY_MODULE_ERROR_ALIGN || shutdown_error == CELL_SPURS_POLICY_MODULE_ERROR_SRCH)
	{
		// printf is used on fw in this case
		cellSpurs.error("cellSpursShutdownTaskset(taskset=*0x%x): Failed with %s (spurs=0x%x, wid=%d)", CellError{ shutdown_error }, spurs, wid);
	}

	return shutdown_error;
}

s32 _spurs::create_task(vm::ptr<CellSpursTaskset> taskset, vm::ptr<u32> task_id, vm::cptr<void> elf, vm::cptr<void> context, u32 size, vm::ptr<CellSpursTaskLsPattern> ls_pattern, vm::ptr<CellSpursTaskArgument> arg)
{
	if (!taskset || !elf)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!elf.aligned(16))
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (_spurs::get_sdk_version() < 0x27FFFF)
	{
		if (!context.aligned(16))
		{
			return CELL_SPURS_TASK_ERROR_ALIGN;
		}
	}
	else
	{
		if (!context.aligned(128))
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

			const u32 ls_blocks = utils::popcnt128(ls_pattern_128._u);

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

	vm::light_op(*vm::_ptr<atomic_be_t<v128>>(taskset.ptr(&CellSpursTaskset::enabled).addr()), [&](atomic_be_t<v128>& ptr)
	{
		// NOTE: Realfw processes this using 4 32-bits atomic loops
		// But here its processed within a single 128-bit atomic op
		ptr.fetch_op([&](be_t<v128>& value)
		{
			auto value0 = value.value();

			if (auto pos = std::countl_one(+value0._u64[0]); pos != 64)
			{
				tmp_task_id = pos;
				value0._u64[0] |= (1ull << 63) >> pos;
				value = value0;
				return true;
			}

			if (auto pos = std::countl_one(+value0._u64[1]); pos != 64)
			{
				tmp_task_id = pos + 64;
				value0._u64[1] |= (1ull << 63) >> pos;
				value = value0;
				return true;
			}

			tmp_task_id = CELL_SPURS_MAX_TASK;
			return false;
		});
	});

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

s32 _spurs::task_start(ppu_thread& ppu, vm::ptr<CellSpursTaskset> taskset, u32 taskId)
{
	vm::light_op(taskset->pending_ready, [&](CellSpursTaskset::atomic_tasks_bitset& v)
	{
		v.values[taskId / 32] |= (1u << 31) >> (taskId % 32);
	});

	auto spurs = +taskset->spurs;
	ppu_execute<&cellSpursSendWorkloadSignal>(ppu, spurs, +taskset->wid);

	if (s32 rc = ppu_execute<&cellSpursWakeUp>(ppu, spurs))
	{
		if (rc + 0u == CELL_SPURS_POLICY_MODULE_ERROR_STAT)
		{
			rc = CELL_SPURS_TASK_ERROR_STAT;
		}
		else
		{
			ensure(rc == CELL_OK);
		}
	}

	return CELL_OK;
}

s32 cellSpursCreateTask(ppu_thread& ppu, vm::ptr<CellSpursTaskset> taskset, vm::ptr<u32> taskId, vm::cptr<void> elf, vm::cptr<void> context, u32 size, vm::ptr<CellSpursTaskLsPattern> lsPattern, vm::ptr<CellSpursTaskArgument> argument)
{
	cellSpurs.warning("cellSpursCreateTask(taskset=*0x%x, taskID=*0x%x, elf=*0x%x, context=*0x%x, size=0x%x, lsPattern=*0x%x, argument=*0x%x)", taskset, taskId, elf, context, size, lsPattern, argument);

	if (!taskset)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!taskset.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	auto rc = _spurs::create_task(taskset, taskId, elf, context, size, lsPattern, argument);
	if (rc != CELL_OK)
	{
		return rc;
	}

	rc = _spurs::task_start(ppu, taskset, *taskId);
	if (rc != CELL_OK)
	{
		return rc;
	}

	return CELL_OK;
}

s32 _cellSpursSendSignal(ppu_thread& ppu, vm::ptr<CellSpursTaskset> taskset, u32 taskId)
{
	cellSpurs.trace("_cellSpursSendSignal(taskset=*0x%x, taskId=0x%x)", taskset, taskId);

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

	int signal;

	vm::reservation_op(ppu, vm::unsafe_ptr_cast<spurs_taskset_signal_op>(taskset), [&](spurs_taskset_signal_op& op)
	{
		const u32 signalled = op.signalled[taskId / 32];
		const u32 running = op.running[taskId / 32];
		const u32 ready = op.ready[taskId / 32];
		const u32 waiting = op.waiting[taskId / 32];
		const u32 enabled = op.enabled[taskId / 32];
		const u32 pready = op.pending_ready[taskId / 32];

		const u32 mask = (1u << 31) >> (taskId % 32);

		if ((running & waiting) || (ready & pready) ||
				((signalled | waiting | pready | running | ready) & ~enabled) || !(enabled & mask))
		{
			// Error conditions:
			// 1) Cannot have a waiting bit and running bit set at the same time
			// 2) Cannot have a read bit and pending_ready bit at the same time
			// 3) Any disabled bit in enabled mask must be not set
			// 4) Specified task must be enabled
			signal = -1;
			return false;
		}

		signal = !!(~signalled & waiting & mask);
		op.signalled[taskId / 32] = signalled | mask;
		return true;
	});

	switch (signal)
	{
	case 0: break;
	case 1:
	{
		auto spurs = +taskset->spurs;

		ppu_execute<&cellSpursSendWorkloadSignal>(ppu, spurs, +taskset->wid);
		auto rc = ppu_execute<&cellSpursWakeUp>(ppu, spurs);
		if (rc + 0u == CELL_SPURS_POLICY_MODULE_ERROR_STAT)
		{
			return CELL_SPURS_TASK_ERROR_STAT;
		}

		return rc;
	}
	default: return CELL_SPURS_TASK_ERROR_SRCH;
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
	cellSpurs.warning("cellSpursTasksetAttributeSetName(attr=*0x%x, name=%s)", attr, name);

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
	cellSpurs.warning("cellSpursTasksetAttributeSetTasksetSize(attr=*0x%x, size=0x%x)", attr, size);

	if (!attr)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (size != sizeof(CellSpursTaskset) && size != sizeof(CellSpursTaskset2))
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	attr->taskset_size = size;
	return CELL_OK;
}

s32 cellSpursTasksetAttributeEnableClearLS(vm::ptr<CellSpursTasksetAttribute> attr, s32 enable)
{
	cellSpurs.warning("cellSpursTasksetAttributeEnableClearLS(attr=*0x%x, enable=%d)", attr, enable);

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
	cellSpurs.warning("_cellSpursTasksetAttribute2Initialize(attribute=*0x%x, revision=%d)", attribute, revision);

	std::memset(attribute.get_ptr(), 0, attribute.size());
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
	cellSpurs.warning("_cellSpursTaskAttribute2Initialize(attribute=*0x%x, revision=%d)", attribute, revision);

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

s32 cellSpursCreateTaskset2(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursTasksetAttribute2> attr)
{
	cellSpurs.warning("cellSpursCreateTaskset2(spurs=*0x%x, taskset=*0x%x, attr=*0x%x)", spurs, taskset, attr);

	vm::var<CellSpursTasksetAttribute2> tmp_attr;

	if (!attr)
	{
		attr = tmp_attr;
		_cellSpursTasksetAttribute2Initialize(attr, 0);
	}

	if (s32 rc = _spurs::create_taskset(ppu, spurs, taskset, attr->args, attr.ptr(&CellSpursTasksetAttribute2::priority), attr->max_contention, attr->name, sizeof(CellSpursTaskset2), attr->enable_clear_ls))
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
	cellSpurs.warning("cellSpursTasksetSetExceptionEventHandler(taskset=*0x%x, handler=*0x%x, arg=*0x%x)", taskset, handler, arg);

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
	cellSpurs.warning("cellSpursTasksetUnsetExceptionEventHandler(taskset=*0x%x)", taskset);

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

s32 cellSpursLookUpTasksetAddress(ppu_thread& /*ppu*/, vm::ptr<CellSpurs> spurs, vm::pptr<CellSpursTaskset> taskset, u32 id)
{
	cellSpurs.warning("cellSpursLookUpTasksetAddress(spurs=*0x%x, taskset=**0x%x, id=0x%x)", spurs, taskset, id);

	if (!taskset)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	vm::var<u64> data;
	if (s32 rc = cellSpursGetWorkloadData(spurs, data, id))
	{
		// Convert policy module error code to a task error code
		return rc ^ 0x100;
	}

	*taskset = vm::cast(*data);
	return CELL_OK;
}

s32 cellSpursTasksetGetSpursAddress(vm::cptr<CellSpursTaskset> taskset, vm::ptr<u32> spurs)
{
	cellSpurs.warning("cellSpursTasksetGetSpursAddress(taskset=*0x%x, spurs=**0x%x)", taskset, spurs);

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

	*spurs = vm::cast(taskset->spurs.addr());
	return CELL_OK;
}

s32 cellSpursGetTasksetInfo()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 _cellSpursTasksetAttributeInitialize(vm::ptr<CellSpursTasksetAttribute> attribute, u32 revision, u32 sdk_version, u64 args, vm::cptr<u8> priority, u32 max_contention)
{
	cellSpurs.warning("_cellSpursTasksetAttributeInitialize(attribute=*0x%x, revision=%d, skd_version=0x%x, args=0x%llx, priority=*0x%x, max_contention=%d)",
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

	std::memset(attribute.get_ptr(), 0, attribute.size());
	attribute->revision = revision;
	attribute->sdk_version = sdk_version;
	attribute->args = args;
	std::memcpy(attribute->priority, priority.get_ptr(), 8);
	attribute->taskset_size = 6400/*CellSpursTaskset::size*/;
	attribute->max_contention = max_contention;
	return CELL_OK;
}

s32 _spurs::check_job_chain_attribute(u32 sdkVer, vm::cptr<u64> jcEntry, u16 sizeJobDescr, u16 maxGrabbedJob,
	u64 priorities, u32 /*maxContention*/, u8 autoSpuCount, u32 tag1, u32 tag2,
	u8 /*isFixedMemAlloc*/, u32 maxSizeJob, u32 initSpuCount)
{
	if (!jcEntry)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!jcEntry.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	if (!maxGrabbedJob || maxGrabbedJob > 0x10u)
		return CELL_SPURS_JOB_ERROR_INVAL;

	// Tag 31 is not allowed from version 1.90 for some reason
	if (u32 max_tag = sdkVer < 0x19000 ? 31 : 30; tag1 > max_tag || tag2 > max_tag)
		return CELL_SPURS_JOB_ERROR_INVAL;

	// Test if any of the value >= CELL_SPURS_MAX_PRIORITY
	if (priorities & 0xf0f0f0f0f0f0f0f0)
		return CELL_SPURS_JOB_ERROR_INVAL;

	if (sizeJobDescr % 0x80 && sizeJobDescr != 64u)
		return CELL_SPURS_JOB_ERROR_INVAL;

	if (autoSpuCount && initSpuCount > 0xffu)
		return CELL_SPURS_JOB_ERROR_INVAL;

	if (maxSizeJob <= 0xffu || maxSizeJob > 0x400u || maxSizeJob % 0x80)
		return CELL_SPURS_JOB_ERROR_INVAL;

	return CELL_OK;
}

s32 _spurs::create_job_chain(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursJobChain> jobChain, vm::cptr<u64> jobChainEntry, u16 /*sizeJob*/,
	u16 maxGrabbedJob, vm::cptr<u8[8]> prio, u32 maxContention, b8 /*autoReadyCount*/,
	u32 tag1, u32 tag2, u32 /*HaltOnError*/, vm::cptr<char> name, u32 /*param_13*/, u32 /*param_14*/)
{
	const s32 sdkVer = _spurs::get_sdk_version();
	jobChain->spurs = spurs;
	jobChain->jmVer = sdkVer > 0x14ffff ? CELL_SPURS_JOB_REVISION_1 : CELL_SPURS_JOB_REVISION_0;

	// Real hack in firmware
	jobChain->val2F = Emu.GetTitleID() == "BLJM60093" ? 1 : 0;
	jobChain->tag1 = static_cast<u8>(tag1);
	jobChain->tag2 = static_cast<u8>(tag2);
	jobChain->isHalted = false;
	jobChain->maxGrabbedJob = maxGrabbedJob;
	jobChain->pc = jobChainEntry;

	auto as_job_error = [](s32 error) -> s32
	{
		switch (error + 0u)
		{
		case CELL_SPURS_POLICY_MODULE_ERROR_AGAIN: return CELL_SPURS_JOB_ERROR_AGAIN;
		case CELL_SPURS_POLICY_MODULE_ERROR_INVAL: return CELL_SPURS_JOB_ERROR_INVAL;
		case CELL_SPURS_POLICY_MODULE_ERROR_STAT: return CELL_SPURS_JOB_ERROR_STAT;
		default: return error;
		}
	};

	vm::var<CellSpursWorkloadAttribute> attr_wkl;
	vm::var<u32> wid;

	// TODO
	if (auto err = _cellSpursWorkloadAttributeInitialize(ppu, +attr_wkl, 1, SYS_PROCESS_PARAM_VERSION_330_0, vm::null, 0, jobChain.addr(), prio, 1, maxContention))
	{
		return as_job_error(err);
	}

	ppu_execute<&cellSpursWorkloadAttributeSetName>(ppu, +attr_wkl, +vm::make_str("JobChain"), name);

	if (auto err = ppu_execute<&cellSpursAddWorkloadWithAttribute>(ppu, spurs, +wid, +attr_wkl))
	{
		return as_job_error(err);
	}

	jobChain->cause = vm::null;
	jobChain->error = 0;
	jobChain->workloadId = *wid;
	return CELL_OK;
}

s32 cellSpursCreateJobChainWithAttribute(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursJobChain> jobChain, vm::ptr<CellSpursJobChainAttribute> attr)
{
	cellSpurs.warning("cellSpursCreateJobChainWithAttribute(spurs=*0x%x, jobChain=*0x%x, attr=*0x%x)", spurs, jobChain, attr);

	if (!attr)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!attr.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	const u64 prio = std::bit_cast<u64>(attr->priorities);

	if (auto err = _spurs::check_job_chain_attribute(attr->sdkVer, attr->jobChainEntry, attr->sizeJobDescriptor, attr->maxGrabbedJob, prio, attr->maxContention
        	, attr->autoSpuCount, attr->tag1, attr->tag2, attr->isFixedMemAlloc, attr->maxSizeJobDescriptor, attr->initSpuCount))
	{
		return err;
	}

	if (!jobChain || !spurs)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!jobChain.aligned() || !spurs.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	std::memset(jobChain.get_ptr(), 0, 0x110);

	// Only allowed revisions in this function
	if (auto ver = attr->jmVer; ver != CELL_SPURS_JOB_REVISION_2 && ver != CELL_SPURS_JOB_REVISION_3)
	{
		return CELL_SPURS_JOB_ERROR_INVAL;
	}

	jobChain->val2C = +attr->isFixedMemAlloc << 7 | (((attr->maxSizeJobDescriptor - 0x100) / 128 & 7) << 4);

	if (auto err = _spurs::create_job_chain(ppu, spurs, jobChain, attr->jobChainEntry, attr->sizeJobDescriptor
			, attr->maxGrabbedJob, attr.ptr(&CellSpursJobChainAttribute::priorities), attr->maxContention, attr->autoSpuCount
			, attr->tag1, attr->tag2, attr->haltOnError, attr->name, 0, 0))
	{
		return err;
	}

	jobChain->initSpuCount = attr->initSpuCount;
	jobChain->jmVer = attr->jmVer;
	jobChain->sdkVer = attr->sdkVer;
	jobChain->jobMemoryCheck = +attr->jobMemoryCheck << 1;
	return CELL_OK;
}

s32 cellSpursCreateJobChain(ppu_thread& ppu, vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursJobChain> jobChain, vm::cptr<u64> jobChainEntry, u16 sizeJobDescriptor
								, u16 maxGrabbedJob, vm::cptr<u8[8]> priorities, u32 maxContention, b8 autoReadyCount, u32 tag1, u32 tag2)
{
	cellSpurs.warning("cellSpursCreateJobChain(spurs=*0x%x, jobChain=*0x%x, jobChainEntry=*0x%x, sizeJobDescriptor=0x%x"
							", maxGrabbedJob=0x%x, priorities=*0x%x, maxContention=%u, autoReadyCount=%s, tag1=%u, %u)", spurs, jobChain, jobChainEntry, sizeJobDescriptor
							, maxGrabbedJob, priorities, maxContention, autoReadyCount, tag1, tag2);

	const u64 prio = std::bit_cast<u64>(*priorities);

	if (auto err = _spurs::check_job_chain_attribute(-1, jobChainEntry, sizeJobDescriptor, maxGrabbedJob, prio, maxContention, autoReadyCount, tag1, tag2, 0, 0, 0))
	{
		return err;
	}

	std::memset(jobChain.get_ptr(), 0, 0x110);

	if (auto err = _spurs::create_job_chain(ppu, spurs, jobChain, jobChainEntry, sizeJobDescriptor, maxGrabbedJob, priorities
			, maxContention, autoReadyCount, tag1, tag2, 0, vm::null, 0, 0))
	{
		return err;
	}

	return CELL_OK;
}

s32 cellSpursJoinJobChain(ppu_thread& ppu, vm::ptr<CellSpursJobChain> jobChain)
{
	cellSpurs.trace("cellSpursJoinJobChain(jobChain=*0x%x)", jobChain);

	if (!jobChain)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!jobChain.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	const u32 wid = jobChain->workloadId;

	if (wid >= CELL_SPURS_MAX_WORKLOAD2)
		return CELL_SPURS_JOB_ERROR_INVAL;

	auto as_job_error = [](s32 error) -> s32
	{
		switch (error + 0u)
		{
		case CELL_SPURS_POLICY_MODULE_ERROR_STAT: return CELL_SPURS_JOB_ERROR_STAT;
		default: return error;
		}
	};

	if (auto err = ppu_execute<&cellSpursWaitForWorkloadShutdown>(ppu, +jobChain->spurs, wid))
	{
		return as_job_error(err);
	}

	if (auto err = ppu_execute<&cellSpursRemoveWorkload>(ppu, +jobChain->spurs, wid))
	{
		// Returned as is
		return err;
	}

	jobChain->workloadId = CELL_SPURS_MAX_WORKLOAD2;
	return jobChain->error;
}

s32 cellSpursKickJobChain(ppu_thread& ppu, vm::ptr<CellSpursJobChain> jobChain, u8 numReadyCount)
{
	cellSpurs.trace("cellSpursKickJobChain(jobChain=*0x%x, numReadyCount=0x%x)", jobChain, numReadyCount);

	if (!jobChain)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!jobChain.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	const u32 wid = jobChain->workloadId;
	const auto spurs = +jobChain->spurs;

	if (wid >= CELL_SPURS_MAX_WORKLOAD2)
		return CELL_SPURS_JOB_ERROR_INVAL;

	if (jobChain->jmVer > CELL_SPURS_JOB_REVISION_1)
		return CELL_SPURS_JOB_ERROR_PERM;

	if (jobChain->autoReadyCount)
		ppu_execute<&cellSpursReadyCountStore>(ppu, spurs, wid, numReadyCount);
	else
		ppu_execute<&cellSpursReadyCountCompareAndSwap>(ppu, spurs, wid, +vm::var<u32>{}, 0, 1);

	const auto err = ppu_execute<&cellSpursWakeUp>(ppu, spurs);

	if (err + 0u == CELL_SPURS_POLICY_MODULE_ERROR_STAT)
	{
		return CELL_SPURS_JOB_ERROR_STAT;
	}

	return err;
}

s32 _cellSpursJobChainAttributeInitialize(u32 jmRevsion, u32 sdkRevision, vm::ptr<CellSpursJobChainAttribute> attr, vm::cptr<u64> jobChainEntry, u16 sizeJobDescriptor, u16 maxGrabbedJob
	, vm::cptr<u8[8]> priorityTable, u32 maxContention, b8 autoRequestSpuCount, u32 tag1, u32 tag2, b8 isFixedMemAlloc, u32 maxSizeJobDescriptor, u32 initialRequestSpuCount)
{
	cellSpurs.trace("_cellSpursJobChainAttributeInitialize(jmRevsion=0x%x, sdkRevision=0x%x, attr=*0x%x, jobChainEntry=*0x%x, sizeJobDescriptor=0x%x, maxGrabbedJob=0x%x, priorityTable=*0x%x"
    	", maxContention=%u, autoRequestSpuCount=%s, tag1=0x%x, tag2=0x%x, isFixedMemAlloc=%s, maxSizeJobDescriptor=0x%x, initialRequestSpuCount=%u)",
		jmRevsion, sdkRevision, attr, jobChainEntry, sizeJobDescriptor, maxGrabbedJob, priorityTable, maxContention, autoRequestSpuCount, tag1, tag2, isFixedMemAlloc, maxSizeJobDescriptor, initialRequestSpuCount);

	if (!attr)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!attr.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	const u64 prio = std::bit_cast<u64>(*priorityTable);

	if (auto err = _spurs::check_job_chain_attribute(sdkRevision, jobChainEntry, sizeJobDescriptor, maxGrabbedJob, prio, maxContention
			, autoRequestSpuCount, tag1, tag2, isFixedMemAlloc, maxSizeJobDescriptor, initialRequestSpuCount))
	{
		return err;
	}

	attr->jmVer = jmRevsion;
	attr->sdkVer = sdkRevision;
	attr->jobChainEntry = jobChainEntry;
	attr->sizeJobDescriptor = sizeJobDescriptor;
	attr->maxGrabbedJob = maxGrabbedJob;
	std::memcpy(&attr->priorities, &prio, 8);
	attr->maxContention = maxContention;
	attr->autoSpuCount = autoRequestSpuCount;
	attr->tag1 = tag1;
	attr->tag2 = tag2;
	attr->isFixedMemAlloc = isFixedMemAlloc;
	attr->maxSizeJobDescriptor = maxSizeJobDescriptor;
	attr->initSpuCount = initialRequestSpuCount;
	attr->haltOnError = 0;
	attr->name = vm::null;
	attr->jobMemoryCheck = false;
	return CELL_OK;
}

s32 cellSpursGetJobChainId(vm::ptr<CellSpursJobChain> jobChain, vm::ptr<u32> id)
{
	cellSpurs.trace("cellSpursGetJobChainId(jobChain=*0x%x, id=*0x%x)", jobChain, id);

	if (!jobChain || !id)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!jobChain.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	*id = jobChain->workloadId;
	return CELL_OK;
}

s32 cellSpursJobChainSetExceptionEventHandler(vm::ptr<CellSpursJobChain> jobChain, vm::ptr<CellSpursJobChainExceptionEventHandler> handler, vm::ptr<void> arg)
{
	cellSpurs.trace("cellSpursJobChainSetExceptionEventHandler(jobChain=*0x%x, handler=*0x%x, arg=*0x%x)", jobChain, handler, arg);

	if (!jobChain || !handler)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!jobChain.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	if (jobChain->workloadId >= CELL_SPURS_MAX_WORKLOAD2)
		return CELL_SPURS_JOB_ERROR_INVAL;

	if (jobChain->exceptionEventHandler)
		return CELL_SPURS_JOB_ERROR_BUSY;

	jobChain->exceptionEventHandlerArgument = arg;
	jobChain->exceptionEventHandler.set(handler.addr());
	return CELL_OK;
}

s32 cellSpursJobChainUnsetExceptionEventHandler(vm::ptr<CellSpursJobChain> jobChain)
{
	cellSpurs.trace("cellSpursJobChainUnsetExceptionEventHandler(jobChain=*0x%x)", jobChain);

	if (!jobChain)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!jobChain.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	if (jobChain->workloadId >= CELL_SPURS_MAX_WORKLOAD2)
		return CELL_SPURS_JOB_ERROR_INVAL;

	jobChain->exceptionEventHandler = vm::null;
	jobChain->exceptionEventHandlerArgument = vm::null;
	return CELL_OK;
}

s32 cellSpursGetJobChainInfo(ppu_thread& ppu, vm::ptr<CellSpursJobChain> jobChain, vm::ptr<CellSpursJobChainInfo> info)
{
	cellSpurs.trace("cellSpursGetJobChainInfo(jobChain=*0x%x, info=*0x%x)", jobChain, info);

	if (!jobChain || !info)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!jobChain.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	const u32 wid = jobChain->workloadId;

	if (wid >= CELL_SPURS_MAX_WORKLOAD2)
		return CELL_SPURS_JOB_ERROR_INVAL;

	vm::var<CellSpursWorkloadInfo> wklInfo;

	if (auto err = ppu_execute<&cellSpursGetWorkloadInfo>(ppu, +jobChain->spurs, wid, +wklInfo))
	{
		if (err + 0u == CELL_SPURS_POLICY_MODULE_ERROR_SRCH)
		{
			return CELL_SPURS_JOB_ERROR_INVAL;
		}

		return err;
	}

	// Read the commands queue atomically
	CellSpursJobChain data;
	vm::peek_op(ppu, vm::unsafe_ptr_cast<CellSpursJobChain_x00>(jobChain), [&](const CellSpursJobChain_x00& jch)
	{
		std::memcpy(&data, &jch, sizeof(jch));
	});

	info->linkRegister[0] = +data.linkRegister[0];
	info->linkRegister[1] = +data.linkRegister[1];
	info->linkRegister[2] = +data.linkRegister[2];
	std::memcpy(&info->urgentCommandSlot, &data.urgentCmds, sizeof(info->urgentCommandSlot));
	info->programCounter = +data.pc;
	info->idWorkload = wid;
	info->maxSizeJobDescriptor = (data.val2C & 0x70u) * 8 + 0x100;
	info->isHalted.set(data.isHalted); // Boolean truncation (non-zero becomes 1)
	info->autoReadyCount.set(data.autoReadyCount);
	info->isFixedMemAlloc = !!(data.val2C & 0x80);
	info->name = wklInfo->nameInstance;
	info->statusCode = jobChain->error;
	info->cause = jobChain->cause;
	info->exceptionEventHandler = +jobChain->exceptionEventHandler;
	info->exceptionEventHandlerArgument = +jobChain->exceptionEventHandlerArgument;
	return CELL_OK;
}

s32 cellSpursJobChainGetSpursAddress(vm::ptr<CellSpursJobChain> jobChain, vm::pptr<CellSpurs> spurs)
{
	cellSpurs.trace("cellSpursJobChainGetSpursAddress(jobChain=*0x%x, spurs=*0x%x)", jobChain, spurs);

	if (!jobChain || !spurs)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!jobChain.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	*spurs = +jobChain->spurs;
	return CELL_OK;
}

s32 cellSpursJobGuardInitialize(vm::ptr<CellSpursJobChain> jobChain, vm::ptr<CellSpursJobGuard> jobGuard, u32 notifyCount, u8 requestSpuCount, u8 autoReset)
{
	cellSpurs.trace("cellSpursJobGuardInitialize(jobChain=*0x%x, jobGuard=*0x%x, notifyCount=0x%x, requestSpuCount=0x%x, autoReset=0x%x)"
		, jobChain, jobGuard, notifyCount, requestSpuCount, autoReset);

	if (!jobChain || !jobGuard)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!jobChain.aligned() || !jobGuard.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	if (jobChain->workloadId >= CELL_SPURS_MAX_WORKLOAD2)
		return CELL_SPURS_JOB_ERROR_INVAL;

	std::memset(jobGuard.get_ptr(), 0, jobGuard.size());
	jobGuard->zero = 0;
	jobGuard->ncount0 = notifyCount;
	jobGuard->ncount1 = notifyCount;
	jobGuard->requestSpuCount = requestSpuCount;
	jobGuard->autoReset = autoReset;
	jobGuard->jobChain = jobChain;
	return CELL_OK;
}

s32 cellSpursJobChainAttributeSetName(vm::ptr<CellSpursJobChainAttribute> attr, vm::cptr<char> name)
{
	cellSpurs.trace("cellSpursJobChainAttributeSetName(attr=*0x%x, name=*0x%x %s)", attr, name, name);

	if (!attr || !name)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!attr.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	attr->name = name;
	return CELL_OK;
}

s32 cellSpursShutdownJobChain(ppu_thread& ppu,vm::ptr<CellSpursJobChain> jobChain)
{
	cellSpurs.trace("cellSpursShutdownJobChain(jobChain=*0x%x)", jobChain);

	if (!jobChain)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!jobChain.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	const u32 wid = jobChain->workloadId;

	if (wid >= CELL_SPURS_MAX_WORKLOAD2)
		return CELL_SPURS_JOB_ERROR_INVAL;

	const auto err = ppu_execute<&cellSpursShutdownWorkload>(ppu, +jobChain->spurs, wid);

	if (err + 0u == CELL_SPURS_POLICY_MODULE_ERROR_STAT)
	{
		return CELL_SPURS_JOB_ERROR_STAT;
	}

	return err;
}

s32 cellSpursJobChainAttributeSetHaltOnError(vm::ptr<CellSpursJobChainAttribute> attr)
{
	cellSpurs.trace("cellSpursJobChainAttributeSetHaltOnError(attr=*0x%x)", attr);

	if (!attr)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!attr.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	attr->haltOnError = true;
	return CELL_OK;
}

s32 cellSpursJobChainAttributeSetJobTypeMemoryCheck(vm::ptr<CellSpursJobChainAttribute> attr)
{
	cellSpurs.trace("cellSpursJobChainAttributeSetJobTypeMemoryCheck(attr=*0x%x)", attr);

	if (!attr)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!attr.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	attr->jobMemoryCheck = true;
	return CELL_OK;
}

s32 cellSpursJobGuardNotify(ppu_thread& ppu, vm::ptr<CellSpursJobGuard> jobGuard)
{
	cellSpurs.trace("cellSpursJobGuardNotify(jobGuard=*0x%x)", jobGuard);

	if (!jobGuard)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!jobGuard.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	u32 allow_jobchain_run = 0; // Affects cellSpursJobChainRun execution
	u32 old = 0;

	const bool ok = vm::reservation_op(ppu, vm::unsafe_ptr_cast<CellSpursJobGuard_x00>(jobGuard), [&](CellSpursJobGuard_x00& jg)
	{
		allow_jobchain_run = jg.zero;
		old = jg.ncount0;

		if (!jg.ncount0)
		{
			return false;
		}

		jg.ncount0--;
		return true;
	});

	if (!ok)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	if (old > 1u)
	{
		return CELL_OK;
	}

	auto jobChain = +jobGuard->jobChain;

	if (jobChain->jmVer <= CELL_SPURS_JOB_REVISION_1)
	{
		ppu_execute<&cellSpursKickJobChain>(ppu, jobChain, static_cast<u8>(jobGuard->requestSpuCount));
	}
	else if (allow_jobchain_run)
	{
		ppu_execute<&cellSpursRunJobChain>(ppu, jobChain);
	}

	return CELL_OK;
}

s32 cellSpursJobGuardReset(vm::ptr<CellSpursJobGuard> jobGuard)
{
	cellSpurs.trace("cellSpursJobGuardReset(jobGuard=*0x%x)", jobGuard);

	if (!jobGuard)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!jobGuard.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	vm::light_op(jobGuard->ncount0, [&](atomic_be_t<u32>& ncount0)
	{
		ncount0 = jobGuard->ncount1;
	});

	return CELL_OK;
}

s32 cellSpursRunJobChain(ppu_thread& ppu, vm::ptr<CellSpursJobChain> jobChain)
{
	cellSpurs.trace("cellSpursRunJobChain(jobChain=*0x%x)", jobChain);

	if (!jobChain)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!jobChain.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	const u32 wid = jobChain->workloadId;

	if (wid >= CELL_SPURS_MAX_WORKLOAD2)
		return CELL_SPURS_JOB_ERROR_INVAL;

	if (jobChain->jmVer <= CELL_SPURS_JOB_REVISION_1)
		return CELL_SPURS_JOB_ERROR_PERM;

	const auto spurs = +jobChain->spurs;
	ppu_execute<&cellSpursSendWorkloadSignal>(ppu, spurs, wid);

	const auto err = ppu_execute<&cellSpursWakeUp>(ppu, spurs);

	if (err + 0u == CELL_SPURS_POLICY_MODULE_ERROR_STAT)
	{
		return CELL_SPURS_JOB_ERROR_STAT;
	}

	return err;
}

s32 cellSpursJobChainGetError(vm::ptr<CellSpursJobChain> jobChain, vm::pptr<void> cause)
{
	cellSpurs.trace("cellSpursJobChainGetError(jobChain=*0x%x, cause=*0x%x)", jobChain, cause);

	if (!jobChain)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!jobChain.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	const s32 error = jobChain->error;
	*cause = (error ? jobChain->cause : vm::null);
	return error;
}

s32 cellSpursGetJobPipelineInfo()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobSetMaxGrab(vm::ptr<CellSpursJobChain> jobChain, u32 maxGrabbedJob)
{
	cellSpurs.trace("cellSpursJobSetMaxGrab(jobChain=*0x%x, maxGrabbedJob=*0x%x)", jobChain, maxGrabbedJob);

	if (!jobChain)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!jobChain.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	if (!maxGrabbedJob || maxGrabbedJob > 0x10u)
		return CELL_SPURS_JOB_ERROR_INVAL;

	const auto spurs = jobChain->spurs;

	// All of these are ERROR_STAT checks unexpectedly
	if (!spurs || !spurs.aligned())
		return CELL_SPURS_JOB_ERROR_STAT;

	const u32 wid = jobChain->workloadId;

	if (wid >= CELL_SPURS_MAX_WORKLOAD2)
		return CELL_SPURS_JOB_ERROR_STAT;

	if ((spurs->wklEnabled & (0x80000000u >> wid)) == 0u)
		return CELL_SPURS_JOB_ERROR_STAT;

	vm::light_op(jobChain->maxGrabbedJob, [&](atomic_be_t<u16>& v)
	{
		v.release(static_cast<u16>(maxGrabbedJob));
	});

	return CELL_OK;
}

s32 cellSpursJobHeaderSetJobbin2Param()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursAddUrgentCommand(ppu_thread& ppu, vm::ptr<CellSpursJobChain> jobChain, u64 newCmd)
{
	cellSpurs.trace("cellSpursAddUrgentCommand(jobChain=*0x%x, newCmd=0x%llx)", jobChain, newCmd);

	if (!jobChain)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!jobChain.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	if (jobChain->workloadId >= CELL_SPURS_MAX_WORKLOAD2)
		return CELL_SPURS_JOB_ERROR_INVAL;

	s32 result = CELL_OK;

	vm::reservation_op(ppu, vm::unsafe_ptr_cast<CellSpursJobChain_x00>(jobChain), [&](CellSpursJobChain_x00& jch)
	{
		for (auto& cmd : jch.urgentCmds)
		{
			if (!cmd)
			{
				cmd = newCmd;
				return true;
			}
		}

		// Considered unlikely so unoptimized
		result = CELL_SPURS_JOB_ERROR_BUSY;
		return false;
	});

	return result;
}

s32 cellSpursAddUrgentCall(ppu_thread& ppu, vm::ptr<CellSpursJobChain> jobChain, vm::ptr<u64> commandList)
{
	cellSpurs.trace("cellSpursAddUrgentCall(jobChain=*0x%x, commandList=*0x%x)", jobChain, commandList);

	if (!commandList)
		return CELL_SPURS_JOB_ERROR_NULL_POINTER;

	if (!commandList.aligned())
		return CELL_SPURS_JOB_ERROR_ALIGN;

	return cellSpursAddUrgentCommand(ppu, jobChain, commandList.addr() | CELL_SPURS_JOB_OPCODE_CALL);
}

s32 cellSpursBarrierInitialize(vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursBarrier> barrier, u32 total)
{
	cellSpurs.trace("cellSpursBarrierInitialize(taskset=*0x%x, barrier=*0x%x, total=0x%x)", taskset, barrier, total);

	if (!taskset || !barrier)
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;

	if (!taskset.aligned() || !barrier.aligned())
		return CELL_SPURS_TASK_ERROR_ALIGN;

	if (!total || total > 128u)
		return CELL_SPURS_TASK_ERROR_INVAL;

	if (taskset->wid >= CELL_SPURS_MAX_WORKLOAD2)
		return CELL_SPURS_TASK_ERROR_INVAL;

	std::memset(barrier.get_ptr(), 0, barrier.size());
	barrier->zero = 0;
	barrier->remained = total;
	barrier->taskset = taskset;
	return CELL_OK;
}

s32 cellSpursBarrierGetTasksetAddress(vm::ptr<CellSpursBarrier> barrier, vm::pptr<CellSpursTaskset> taskset)
{
	cellSpurs.trace("cellSpursBarrierGetTasksetAddress(barrier=*0x%x, taskset=*0x%x)", barrier, taskset);

	if (!taskset || !barrier)
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;

	if (!barrier.aligned())
		return CELL_SPURS_TASK_ERROR_ALIGN;

	*taskset = barrier->taskset;
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

DECLARE(ppu_module_manager::cellSpurs)("cellSpurs", [](ppu_static_module* _this)
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
	REG_FUNC(cellSpurs, cellSpursRemoveSystemWorkloadForUtility);
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

	_this->add_init_func([](ppu_static_module*)
	{
		const auto val = g_cfg.core.spu_accurate_reservations ? MFF_PERFECT : MFF_FORCED_HLE;

		REINIT_FUNC(cellSpursSetPriorities).flag(val);
		REINIT_FUNC(cellSpursAddWorkload).flag(val);
		REINIT_FUNC(cellSpursAddWorkloadWithAttribute).flag(val);
		REINIT_FUNC(cellSpursShutdownWorkload).flag(val);
		REINIT_FUNC(cellSpursReadyCountStore).flag(val);
		REINIT_FUNC(cellSpursSetPriority).flag(val);
		REINIT_FUNC(cellSpursTraceInitialize).flag(val);
		REINIT_FUNC(cellSpursWaitForWorkloadShutdown).flag(val);
		REINIT_FUNC(cellSpursRequestIdleSpu).flag(val);
		REINIT_FUNC(cellSpursRemoveWorkload).flag(val);
	});
});
