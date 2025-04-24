#include "stdafx.h"
#include "Loader/ELF.h"

#include "Emu/Memory/vm_reservation.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/SPURecompiler.h"
#include "cellSpurs.h"

#include "util/asm.hpp"
#include "util/v128.hpp"
#include "util/simd.hpp"

LOG_CHANNEL(cellSpurs);

// Temporarily
#ifndef _MSC_VER
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

//----------------------------------------------------------------------------
// Function prototypes
//----------------------------------------------------------------------------

//
// SPURS utility functions
//
static void cellSpursModulePutTrace(CellSpursTracePacket* packet, u32 dmaTagId);
static u32 cellSpursModulePollStatus(spu_thread& spu, u32* status);
static void cellSpursModuleExit(spu_thread& spu);

static bool spursDma(spu_thread& spu, u32 cmd, u64 ea, u32 lsa, u32 size, u32 tag);
static u32 spursDmaGetCompletionStatus(spu_thread& spu, u32 tagMask);
static u32 spursDmaWaitForCompletion(spu_thread& spu, u32 tagMask, bool waitForAll = true);
static void spursHalt(spu_thread& spu);

//
// SPURS kernel functions
//
static bool spursKernel1SelectWorkload(spu_thread& spu);
static bool spursKernel2SelectWorkload(spu_thread& spu);
static void spursKernelDispatchWorkload(spu_thread& spu, u64 widAndPollStatus);
static bool spursKernelWorkloadExit(spu_thread& spu);
bool spursKernelEntry(spu_thread& spu);

//
// SPURS system workload functions
//
static bool spursSysServiceEntry(spu_thread& spu);
// TODO: Exit
static void spursSysServiceIdleHandler(spu_thread& spu, SpursKernelContext* ctxt);
static void spursSysServiceMain(spu_thread& spu, u32 pollStatus);
static void spursSysServiceProcessRequests(spu_thread& spu, SpursKernelContext* ctxt);
static void spursSysServiceActivateWorkload(spu_thread& spu, SpursKernelContext* ctxt);
// TODO: Deactivate workload
static void spursSysServiceUpdateShutdownCompletionEvents(spu_thread& spu, SpursKernelContext* ctxt, u32 wklShutdownBitSet);
static void spursSysServiceTraceSaveCount(spu_thread& spu, SpursKernelContext* ctxt);
static void spursSysServiceTraceUpdate(spu_thread& spu, SpursKernelContext* ctxt, u32 arg2, u32 arg3, u32 forceNotify);
// TODO: Deactivate trace
// TODO: System workload entry
static void spursSysServiceCleanupAfterSystemWorkload(spu_thread& spu, SpursKernelContext* ctxt);

//
// SPURS taskset policy module functions
//
static bool spursTasksetEntry(spu_thread& spu);
static bool spursTasksetSyscallEntry(spu_thread& spu);
static void spursTasksetResumeTask(spu_thread& spu);
static void spursTasksetStartTask(spu_thread& spu, CellSpursTaskArgument& taskArgs);
static s32 spursTasksetProcessRequest(spu_thread& spu, s32 request, u32* taskId, u32* isWaiting);
static void spursTasksetProcessPollStatus(spu_thread& spu, u32 pollStatus);
static bool spursTasksetPollStatus(spu_thread& spu);
static void spursTasksetExit(spu_thread& spu);
static void spursTasksetOnTaskExit(spu_thread& spu, u64 addr, u32 taskId, s32 exitCode, u64 args);
static s32 spursTasketSaveTaskContext(spu_thread& spu);
static void spursTasksetDispatch(spu_thread& spu);
static s32 spursTasksetProcessSyscall(spu_thread& spu, u32 syscallNum, u32 args);
static void spursTasksetInit(spu_thread& spu, u32 pollStatus);
static s32 spursTasksetLoadElf(spu_thread& spu, u32* entryPoint, u32* lowestLoadAddr, u64 elfAddr, bool skipWriteableSegments);

//
// SPURS jobchain policy module functions
//
bool spursJobChainEntry(spu_thread& spu);
void spursJobchainPopUrgentCommand(spu_thread& spu);

//----------------------------------------------------------------------------
// SPURS utility functions
//----------------------------------------------------------------------------

// Output trace information
void cellSpursModulePutTrace(CellSpursTracePacket* /*packet*/, u32 /*dmaTagId*/)
{
	// TODO: Implement this
}

// Check for execution right requests
u32 cellSpursModulePollStatus(spu_thread& spu, u32* status)
{
	auto ctxt = spu._ptr<SpursKernelContext>(0x100);

	spu.gpr[3]._u32[3] = 1;
	if (ctxt->spurs->flags1 & SF1_32_WORKLOADS)
	{
		spursKernel2SelectWorkload(spu);
	}
	else
	{
		spursKernel1SelectWorkload(spu);
	}

	auto result = spu.gpr[3]._u64[1];
	if (status)
	{
		*status = static_cast<u32>(result);
	}

	u32 wklId = result >> 32;
	return wklId == ctxt->wklCurrentId ? 0 : 1;
}

// Exit current workload
void cellSpursModuleExit(spu_thread& spu)
{
	auto ctxt = spu._ptr<SpursKernelContext>(0x100);
	spu.pc = ctxt->exitToKernelAddr;

	// TODO: use g_escape for actual long jump
	//throw SpursModuleExit();
}

// Execute a DMA operation
bool spursDma(spu_thread& spu, const spu_mfc_cmd& args)
{
	spu.ch_mfc_cmd = args;

	if (!spu.process_mfc_cmd())
	{
		spu_runtime::g_escape(&spu);
	}

	if (args.cmd == MFC_GETLLAR_CMD || args.cmd == MFC_PUTLLC_CMD || args.cmd == MFC_PUTLLUC_CMD)
	{
		return static_cast<u32>(spu.get_ch_value(MFC_RdAtomicStat)) != MFC_PUTLLC_FAILURE;
	}

	return true;
}

// Execute a DMA operation
bool spursDma(spu_thread& spu, u32 cmd, u64 ea, u32 lsa, u32 size, u32 tag)
{
	return spursDma(spu, {MFC(cmd), static_cast<u8>(tag & 0x1f), static_cast<u16>(size & 0x7fff), lsa, static_cast<u32>(ea), static_cast<u32>(ea >> 32)});
}

// Get the status of DMA operations
u32 spursDmaGetCompletionStatus(spu_thread& spu, u32 tagMask)
{
	spu.set_ch_value(MFC_WrTagMask, tagMask);
	spu.set_ch_value(MFC_WrTagUpdate, MFC_TAG_UPDATE_IMMEDIATE);
	return static_cast<u32>(spu.get_ch_value(MFC_RdTagStat));
}

// Wait for DMA operations to complete
u32 spursDmaWaitForCompletion(spu_thread& spu, u32 tagMask, bool waitForAll)
{
	spu.set_ch_value(MFC_WrTagMask, tagMask);
	spu.set_ch_value(MFC_WrTagUpdate, waitForAll ? MFC_TAG_UPDATE_ALL : MFC_TAG_UPDATE_ANY);
	return static_cast<u32>(spu.get_ch_value(MFC_RdTagStat));
}

// Halt the SPU
void spursHalt(spu_thread& spu)
{
	spu.halt();
}

void sys_spu_thread_exit(spu_thread& spu, s32 status)
{
	// Cancel any pending status update requests
	spu.set_ch_value(MFC_WrTagUpdate, 0);
	while (spu.get_ch_count(MFC_RdTagStat) != 1);
	spu.get_ch_value(MFC_RdTagStat);

	// Wait for all pending DMA operations to complete
	spu.set_ch_value(MFC_WrTagMask, 0xFFFFFFFF);
	spu.set_ch_value(MFC_WrTagUpdate, MFC_TAG_UPDATE_ALL);
	spu.get_ch_value(MFC_RdTagStat);

	spu.set_ch_value(SPU_WrOutMbox, status);
	spu.stop_and_signal(0x102);
}

void sys_spu_thread_group_exit(spu_thread& spu, s32 status)
{
	// Cancel any pending status update requests
	spu.set_ch_value(MFC_WrTagUpdate, 0);
	while (spu.get_ch_count(MFC_RdTagStat) != 1);
	spu.get_ch_value(MFC_RdTagStat);

	// Wait for all pending DMA operations to complete
	spu.set_ch_value(MFC_WrTagMask, 0xFFFFFFFF);
	spu.set_ch_value(MFC_WrTagUpdate, MFC_TAG_UPDATE_ALL);
	spu.get_ch_value(MFC_RdTagStat);

	spu.set_ch_value(SPU_WrOutMbox, status);
	spu.stop_and_signal(0x101);
}

s32 sys_spu_thread_send_event(spu_thread& spu, u8 spup, u32 data0, u32 data1)
{
	if (spup > 0x3F)
	{
		return CELL_EINVAL;
	}

	if (spu.get_ch_count(SPU_RdInMbox))
	{
		return CELL_EBUSY;
	}

	spu.set_ch_value(SPU_WrOutMbox, data1);
	spu.set_ch_value(SPU_WrOutIntrMbox, (spup << 24) | (data0 & 0x00FFFFFF));
	return static_cast<u32>(spu.get_ch_value(SPU_RdInMbox));
}

s32 sys_spu_thread_switch_system_module(spu_thread& spu, u32 status)
{
	if (spu.get_ch_count(SPU_RdInMbox))
	{
		return CELL_EBUSY;
	}

	u32 result;

	// Cancel any pending status update requests
	spu.set_ch_value(MFC_WrTagUpdate, 0);
	while (spu.get_ch_count(MFC_RdTagStat) != 1);
	spu.get_ch_value(MFC_RdTagStat);

	// Wait for all pending DMA operations to complete
	spu.set_ch_value(MFC_WrTagMask, 0xFFFFFFFF);
	spu.set_ch_value(MFC_WrTagUpdate, MFC_TAG_UPDATE_ALL);
	spu.get_ch_value(MFC_RdTagStat);

	do
	{
		spu.set_ch_value(SPU_WrOutMbox, status);
		spu.stop_and_signal(0x120);
		result = static_cast<u32>(spu.get_ch_value(SPU_RdInMbox));
	}
	while (result == CELL_EBUSY);

	return result;
}

//----------------------------------------------------------------------------
// SPURS kernel functions
//----------------------------------------------------------------------------

// Select a workload to run
bool spursKernel1SelectWorkload(spu_thread& spu)
{
	const auto ctxt = spu._ptr<SpursKernelContext>(0x100);

	// The first and only argument to this function is a boolean that is set to false if the function
	// is called by the SPURS kernel and set to true if called by cellSpursModulePollStatus.
	// If the first argument is true then the shared data is not updated with the result.
	const auto isPoll = spu.gpr[3]._u32[3];

	u32 wklSelectedId;
	u32 pollStatus;

	//vm::reservation_op(vm::cast(ctxt->spurs.addr()), 128, [&]()
	{
		// lock the first 0x80 bytes of spurs
		auto spurs = ctxt->spurs.get_ptr();

		// Calculate the contention (number of SPUs used) for each workload
		u8 contention[CELL_SPURS_MAX_WORKLOAD];
		u8 pendingContention[CELL_SPURS_MAX_WORKLOAD];
		for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++)
		{
			contention[i] = spurs->wklCurrentContention[i] - ctxt->wklLocContention[i];

			// If this is a poll request then the number of SPUs pending to context switch is also added to the contention presumably
			// to prevent unnecessary jumps to the kernel
			if (isPoll)
			{
				pendingContention[i] = spurs->wklPendingContention[i] - ctxt->wklLocPendingContention[i];
				if (i != ctxt->wklCurrentId)
				{
					contention[i] += pendingContention[i];
				}
			}
		}

		wklSelectedId = CELL_SPURS_SYS_SERVICE_WORKLOAD_ID;
		pollStatus = 0;

		// The system service has the highest priority. Select the system service if
		// the system service message bit for this SPU is set.
		if (spurs->sysSrvMessage & (1 << ctxt->spuNum))
		{
			ctxt->spuIdling = 0;
			if (!isPoll || ctxt->wklCurrentId == CELL_SPURS_SYS_SERVICE_WORKLOAD_ID)
			{
				// Clear the message bit
				spurs->sysSrvMessage.raw() &= ~(1 << ctxt->spuNum);
			}
		}
		else
		{
			// Caclulate the scheduling weight for each workload
			u16 maxWeight = 0;
			for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++)
			{
				u16 runnable = ctxt->wklRunnable1 & (0x8000 >> i);
				u16 wklSignal = spurs->wklSignal1.load() & (0x8000 >> i);
				u8  wklFlag = spurs->wklFlag.flag.load() == 0u ? spurs->wklFlagReceiver == i ? 1 : 0 : 0;
				u8  readyCount = spurs->wklReadyCount1[i] > CELL_SPURS_MAX_SPU ? CELL_SPURS_MAX_SPU : spurs->wklReadyCount1[i].load();
				u8  idleSpuCount = spurs->wklIdleSpuCountOrReadyCount2[i] > CELL_SPURS_MAX_SPU ? CELL_SPURS_MAX_SPU : spurs->wklIdleSpuCountOrReadyCount2[i].load();
				u8  requestCount = readyCount + idleSpuCount;

				// For a workload to be considered for scheduling:
				// 1. Its priority must not be 0
				// 2. The number of SPUs used by it must be less than the max contention for that workload
				// 3. The workload should be in runnable state
				// 4. The number of SPUs allocated to it must be less than the number of SPUs requested (i.e. readyCount)
				//    OR the workload must be signalled
				//    OR the workload flag is 0 and the workload is configured as the wokload flag receiver
				if (runnable && ctxt->priority[i] != 0 && spurs->wklMaxContention[i] > contention[i])
				{
					if (wklFlag || wklSignal || (readyCount != 0 && requestCount > contention[i]))
					{
						// The scheduling weight of the workload is formed from the following parameters in decreasing order of priority:
						// 1. Wokload signal set or workload flag or ready count > contention
						// 2. Priority of the workload on the SPU
						// 3. Is the workload the last selected workload
						// 4. Minimum contention of the workload
						// 5. Number of SPUs that are being used by the workload (lesser the number, more the weight)
						// 6. Is the workload executable same as the currently loaded executable
						// 7. The workload id (lesser the number, more the weight)
						u16 weight = (wklFlag || wklSignal || (readyCount > contention[i])) ? 0x8000 : 0;
						weight |= (ctxt->priority[i] & 0x7F) << 8; // TODO: was shifted << 16
						weight |= i == ctxt->wklCurrentId ? 0x80 : 0x00;
						weight |= (contention[i] > 0 && spurs->wklMinContention[i] > contention[i]) ? 0x40 : 0x00;
						weight |= ((CELL_SPURS_MAX_SPU - contention[i]) & 0x0F) << 2;
						weight |= ctxt->wklUniqueId[i] == ctxt->wklCurrentId ? 0x02 : 0x00;
						weight |= 0x01;

						// In case of a tie the lower numbered workload is chosen
						if (weight > maxWeight)
						{
							wklSelectedId = i;
							maxWeight = weight;
							pollStatus = readyCount > contention[i] ? CELL_SPURS_MODULE_POLL_STATUS_READYCOUNT : 0;
							pollStatus |= wklSignal ? CELL_SPURS_MODULE_POLL_STATUS_SIGNAL : 0;
							pollStatus |= wklFlag ? CELL_SPURS_MODULE_POLL_STATUS_FLAG : 0;
						}
					}
				}
			}

			// Not sure what this does. Possibly mark the SPU as idle/in use.
			ctxt->spuIdling = wklSelectedId == CELL_SPURS_SYS_SERVICE_WORKLOAD_ID ? 1 : 0;

			if (!isPoll || wklSelectedId == ctxt->wklCurrentId)
			{
				// Clear workload signal for the selected workload
				spurs->wklSignal1.raw() &= ~(0x8000 >> wklSelectedId);
				spurs->wklSignal2.raw() &= ~(0x80000000u >> wklSelectedId);

				// If the selected workload is the wklFlag workload then pull the wklFlag to all 1s
				if (wklSelectedId == spurs->wklFlagReceiver)
				{
					spurs->wklFlag.flag = -1;
				}
			}
		}

		if (!isPoll)
		{
			// Called by kernel
			// Increment the contention for the selected workload
			if (wklSelectedId != CELL_SPURS_SYS_SERVICE_WORKLOAD_ID)
			{
				contention[wklSelectedId]++;
			}

			for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++)
			{
				spurs->wklCurrentContention[i] = contention[i];
				spurs->wklPendingContention[i] = spurs->wklPendingContention[i] - ctxt->wklLocPendingContention[i];
				ctxt->wklLocContention[i] = 0;
				ctxt->wklLocPendingContention[i] = 0;
			}

			if (wklSelectedId != CELL_SPURS_SYS_SERVICE_WORKLOAD_ID)
			{
				ctxt->wklLocContention[wklSelectedId] = 1;
			}

			ctxt->wklCurrentId = wklSelectedId;
		}
		else if (wklSelectedId != ctxt->wklCurrentId)
		{
			// Not called by kernel but a context switch is required
			// Increment the pending contention for the selected workload
			if (wklSelectedId != CELL_SPURS_SYS_SERVICE_WORKLOAD_ID)
			{
				pendingContention[wklSelectedId]++;
			}

			for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++)
			{
				spurs->wklPendingContention[i] = pendingContention[i];
				ctxt->wklLocPendingContention[i] = 0;
			}

			if (wklSelectedId != CELL_SPURS_SYS_SERVICE_WORKLOAD_ID)
			{
				ctxt->wklLocPendingContention[wklSelectedId] = 1;
			}
		}
		else
		{
			// Not called by kernel and no context switch is required
			for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++)
			{
				spurs->wklPendingContention[i] = spurs->wklPendingContention[i] - ctxt->wklLocPendingContention[i];
				ctxt->wklLocPendingContention[i] = 0;
			}
		}

		std::memcpy(ctxt, spurs, 128);
	}//);

	u64 result = u64{wklSelectedId} << 32;
	result |= pollStatus;
	spu.gpr[3]._u64[1] = result;
	return true;
}

// Select a workload to run
bool spursKernel2SelectWorkload(spu_thread& spu)
{
	const auto ctxt = spu._ptr<SpursKernelContext>(0x100);

	// The first and only argument to this function is a boolean that is set to false if the function
	// is called by the SPURS kernel and set to true if called by cellSpursModulePollStatus.
	// If the first argument is true then the shared data is not updated with the result.
	const auto isPoll = spu.gpr[3]._u32[3];

	u32 wklSelectedId;
	u32 pollStatus;

	//vm::reservation_op(vm::cast(ctxt->spurs.addr()), 128, [&]()
	{
		// lock the first 0x80 bytes of spurs
		auto spurs = ctxt->spurs.get_ptr();

		// Calculate the contention (number of SPUs used) for each workload
		u8 contention[CELL_SPURS_MAX_WORKLOAD2];
		u8 pendingContention[CELL_SPURS_MAX_WORKLOAD2];
		for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD2; i++)
		{
			contention[i] = spurs->wklCurrentContention[i & 0x0F] - ctxt->wklLocContention[i & 0x0F];
			contention[i] = i + 0u < CELL_SPURS_MAX_WORKLOAD ? contention[i] & 0x0F : contention[i] >> 4;

			// If this is a poll request then the number of SPUs pending to context switch is also added to the contention presumably
			// to prevent unnecessary jumps to the kernel
			if (isPoll)
			{
				pendingContention[i] = spurs->wklPendingContention[i & 0x0F] - ctxt->wklLocPendingContention[i & 0x0F];
				pendingContention[i] = i + 0u < CELL_SPURS_MAX_WORKLOAD ? pendingContention[i] & 0x0F : pendingContention[i] >> 4;
				if (i != ctxt->wklCurrentId)
				{
					contention[i] += pendingContention[i];
				}
			}
		}

		wklSelectedId = CELL_SPURS_SYS_SERVICE_WORKLOAD_ID;
		pollStatus = 0;

		// The system service has the highest priority. Select the system service if
		// the system service message bit for this SPU is set.
		if (spurs->sysSrvMessage & (1 << ctxt->spuNum))
		{
			// Not sure what this does. Possibly Mark the SPU as in use.
			ctxt->spuIdling = 0;
			if (!isPoll || ctxt->wklCurrentId == CELL_SPURS_SYS_SERVICE_WORKLOAD_ID)
			{
				// Clear the message bit
				spurs->sysSrvMessage.raw() &= ~(1 << ctxt->spuNum);
			}
		}
		else
		{
			// Caclulate the scheduling weight for each workload
			u8 maxWeight = 0;
			for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD2; i++)
			{
				u32 j = i & 0x0f;
				u16 runnable = i < CELL_SPURS_MAX_WORKLOAD ? ctxt->wklRunnable1 & (0x8000 >> j) : ctxt->wklRunnable2 & (0x8000 >> j);
				u8  priority = i < CELL_SPURS_MAX_WORKLOAD ? ctxt->priority[j] & 0x0F : ctxt->priority[j] >> 4;
				u8  maxContention = i < CELL_SPURS_MAX_WORKLOAD ? spurs->wklMaxContention[j] & 0x0F : spurs->wklMaxContention[j] >> 4;
				u16 wklSignal = i < CELL_SPURS_MAX_WORKLOAD ? spurs->wklSignal1.load() & (0x8000 >> j) : spurs->wklSignal2.load() & (0x8000 >> j);
				u8  wklFlag = spurs->wklFlag.flag.load() == 0u ? spurs->wklFlagReceiver == i ? 1 : 0 : 0;
				u8  readyCount = i < CELL_SPURS_MAX_WORKLOAD ? spurs->wklReadyCount1[j] : spurs->wklIdleSpuCountOrReadyCount2[j];

				// For a workload to be considered for scheduling:
				// 1. Its priority must be greater than 0
				// 2. The number of SPUs used by it must be less than the max contention for that workload
				// 3. The workload should be in runnable state
				// 4. The number of SPUs allocated to it must be less than the number of SPUs requested (i.e. readyCount)
				//    OR the workload must be signalled
				//    OR the workload flag is 0 and the workload is configured as the wokload receiver
				if (runnable && priority > 0 && maxContention > contention[i])
				{
					if (wklFlag || wklSignal || readyCount > contention[i])
					{
						// The scheduling weight of the workload is equal to the priority of the workload for the SPU.
						// The current workload is given a sligtly higher weight presumably to reduce the number of context switches.
						// In case of a tie the lower numbered workload is chosen.
						u8 weight = priority << 4;
						if (ctxt->wklCurrentId == i)
						{
							weight |= 0x04;
						}

						if (weight > maxWeight)
						{
							wklSelectedId = i;
							maxWeight = weight;
							pollStatus = readyCount > contention[i] ? CELL_SPURS_MODULE_POLL_STATUS_READYCOUNT : 0;
							pollStatus |= wklSignal ? CELL_SPURS_MODULE_POLL_STATUS_SIGNAL : 0;
							pollStatus |= wklFlag ? CELL_SPURS_MODULE_POLL_STATUS_FLAG : 0;
						}
					}
				}
			}

			// Not sure what this does. Possibly mark the SPU as idle/in use.
			ctxt->spuIdling = wklSelectedId == CELL_SPURS_SYS_SERVICE_WORKLOAD_ID ? 1 : 0;

			if (!isPoll || wklSelectedId == ctxt->wklCurrentId)
			{
				// Clear workload signal for the selected workload
				spurs->wklSignal1.raw() &= ~(0x8000 >> wklSelectedId);
				spurs->wklSignal2.raw() &= ~(0x80000000u >> wklSelectedId);

				// If the selected workload is the wklFlag workload then pull the wklFlag to all 1s
				if (wklSelectedId == spurs->wklFlagReceiver)
				{
					spurs->wklFlag.flag = -1;
				}
			}
		}

		if (!isPoll)
		{
			// Called by kernel
			// Increment the contention for the selected workload
			if (wklSelectedId != CELL_SPURS_SYS_SERVICE_WORKLOAD_ID)
			{
				contention[wklSelectedId]++;
			}

			for (u32 i = 0; i < (CELL_SPURS_MAX_WORKLOAD2 >> 1); i++)
			{
				spurs->wklCurrentContention[i] = contention[i] | (contention[i + 0x10] << 4);
				spurs->wklPendingContention[i] = spurs->wklPendingContention[i] - ctxt->wklLocPendingContention[i];
				ctxt->wklLocContention[i] = 0;
				ctxt->wklLocPendingContention[i] = 0;
			}

			ctxt->wklLocContention[wklSelectedId & 0x0F] = wklSelectedId < CELL_SPURS_MAX_WORKLOAD ? 0x01 : wklSelectedId < CELL_SPURS_MAX_WORKLOAD2 ? 0x10 : 0;
			ctxt->wklCurrentId = wklSelectedId;
		}
		else if (wklSelectedId != ctxt->wklCurrentId)
		{
			// Not called by kernel but a context switch is required
			// Increment the pending contention for the selected workload
			if (wklSelectedId != CELL_SPURS_SYS_SERVICE_WORKLOAD_ID)
			{
				pendingContention[wklSelectedId]++;
			}

			for (u32 i = 0; i < (CELL_SPURS_MAX_WORKLOAD2 >> 1); i++)
			{
				spurs->wklPendingContention[i] = pendingContention[i] | (pendingContention[i + 0x10] << 4);
				ctxt->wklLocPendingContention[i] = 0;
			}

			ctxt->wklLocPendingContention[wklSelectedId & 0x0F] = wklSelectedId < CELL_SPURS_MAX_WORKLOAD ? 0x01 : wklSelectedId < CELL_SPURS_MAX_WORKLOAD2 ? 0x10 : 0;
		}
		else
		{
			// Not called by kernel and no context switch is required
			for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++)
			{
				spurs->wklPendingContention[i] = spurs->wklPendingContention[i] - ctxt->wklLocPendingContention[i];
				ctxt->wklLocPendingContention[i] = 0;
			}
		}

		std::memcpy(ctxt, spurs, 128);
	}//);

	u64 result = u64{wklSelectedId} << 32;
	result |= pollStatus;
	spu.gpr[3]._u64[1] = result;
	return true;
}

// SPURS kernel dispatch workload
void spursKernelDispatchWorkload(spu_thread& spu, u64 widAndPollStatus)
{
	const auto ctxt = spu._ptr<SpursKernelContext>(0x100);
	const bool isKernel2 = ctxt->spurs->flags1 & SF1_32_WORKLOADS ? true : false;

	auto pollStatus = static_cast<u32>(widAndPollStatus);
	auto wid = static_cast<u32>(widAndPollStatus >> 32);

	// DMA in the workload info for the selected workload
	auto wklInfoOffset = wid < CELL_SPURS_MAX_WORKLOAD ? &ctxt->spurs->wklInfo1[wid] :
		wid < CELL_SPURS_MAX_WORKLOAD2 && isKernel2 ? &ctxt->spurs->wklInfo2[wid & 0xf] :
		&ctxt->spurs->wklInfoSysSrv;

	const auto wklInfo = spu._ptr<CellSpurs::WorkloadInfo>(0x3FFE0);
	std::memcpy(wklInfo, wklInfoOffset, 0x20);

	// Load the workload to LS
	if (ctxt->wklCurrentAddr != wklInfo->addr)
	{
		switch (wklInfo->addr.addr())
		{
		case SPURS_IMG_ADDR_SYS_SRV_WORKLOAD:
			//spu.RegisterHleFunction(0xA00, spursSysServiceEntry);
			break;
		case SPURS_IMG_ADDR_TASKSET_PM:
			//spu.RegisterHleFunction(0xA00, spursTasksetEntry);
			break;
		default:
			std::memcpy(spu._ptr<void>(0xA00), wklInfo->addr.get_ptr(), wklInfo->size);
			break;
		}

		ctxt->wklCurrentAddr = wklInfo->addr;
		ctxt->wklCurrentUniqueId = wklInfo->uniqueId;
	}

	if (!isKernel2)
	{
		ctxt->moduleId[0] = 0;
		ctxt->moduleId[1] = 0;
	}

	// Run workload
	spu.gpr[0]._u32[3] = ctxt->exitToKernelAddr;
	spu.gpr[1]._u32[3] = 0x3FFB0;
	spu.gpr[3]._u32[3] = 0x100;
	spu.gpr[4]._u64[1] = wklInfo->arg;
	spu.gpr[5]._u32[3] = pollStatus;
	spu.pc = 0xA00;
}

// SPURS kernel workload exit
bool spursKernelWorkloadExit(spu_thread& spu)
{
	const auto ctxt = spu._ptr<SpursKernelContext>(0x100);
	const bool isKernel2 = ctxt->spurs->flags1 & SF1_32_WORKLOADS ? true : false;

	// Select next workload to run
	spu.gpr[3].clear();
	if (isKernel2)
	{
		spursKernel2SelectWorkload(spu);
	}
	else
	{
		spursKernel1SelectWorkload(spu);
	}

	spursKernelDispatchWorkload(spu, spu.gpr[3]._u64[1]);
	return false;
}

// SPURS kernel entry point
bool spursKernelEntry(spu_thread& spu)
{
	const auto ctxt = spu._ptr<SpursKernelContext>(0x100);
	memset(ctxt, 0, sizeof(SpursKernelContext));

	// Save arguments
	ctxt->spuNum = spu.gpr[3]._u32[3];
	ctxt->spurs.set(spu.gpr[4]._u64[1]);

	const bool isKernel2 = ctxt->spurs->flags1 & SF1_32_WORKLOADS ? true : false;

	// Initialise the SPURS context to its initial values
	ctxt->dmaTagId = CELL_SPURS_KERNEL_DMA_TAG_ID;
	ctxt->wklCurrentUniqueId = 0x20;
	ctxt->wklCurrentId = CELL_SPURS_SYS_SERVICE_WORKLOAD_ID;
	ctxt->exitToKernelAddr = isKernel2 ? CELL_SPURS_KERNEL2_EXIT_ADDR : CELL_SPURS_KERNEL1_EXIT_ADDR;
	ctxt->selectWorkloadAddr = isKernel2 ? CELL_SPURS_KERNEL2_SELECT_WORKLOAD_ADDR : CELL_SPURS_KERNEL1_SELECT_WORKLOAD_ADDR;
	if (!isKernel2)
	{
		ctxt->x1F0 = 0xF0020000;
		ctxt->x200 = 0x20000;
		ctxt->guid[0] = 0x423A3A02;
		ctxt->guid[1] = 0x43F43A82;
		ctxt->guid[2] = 0x43F26502;
		ctxt->guid[3] = 0x420EB382;
	}
	else
	{
		ctxt->guid[0] = 0x43A08402;
		ctxt->guid[1] = 0x43FB0A82;
		ctxt->guid[2] = 0x435E9302;
		ctxt->guid[3] = 0x43A3C982;
	}

	// Register SPURS kernel HLE functions
	//spu.UnregisterHleFunctions(0, 0x40000/*LS_BOTTOM*/);
	//spu.RegisterHleFunction(isKernel2 ? CELL_SPURS_KERNEL2_ENTRY_ADDR : CELL_SPURS_KERNEL1_ENTRY_ADDR, spursKernelEntry);
	//spu.RegisterHleFunction(ctxt->exitToKernelAddr, spursKernelWorkloadExit);
	//spu.RegisterHleFunction(ctxt->selectWorkloadAddr, isKernel2 ? spursKernel2SelectWorkload : spursKernel1SelectWorkload);

	// Start the system service
	spursKernelDispatchWorkload(spu, u64{CELL_SPURS_SYS_SERVICE_WORKLOAD_ID} << 32);
	return false;
}

//----------------------------------------------------------------------------
// SPURS system workload functions
//----------------------------------------------------------------------------

// Entry point of the system service
bool spursSysServiceEntry(spu_thread& spu)
{
	const auto ctxt = spu._ptr<SpursKernelContext>(spu.gpr[3]._u32[3]);
	//auto arg = spu.gpr[4]._u64[1];
	auto pollStatus = spu.gpr[5]._u32[3];

	{
		if (ctxt->wklCurrentId == CELL_SPURS_SYS_SERVICE_WORKLOAD_ID)
		{
			spursSysServiceMain(spu, pollStatus);
		}
		else
		{
			// TODO: If we reach here it means the current workload was preempted to start the
			// system workload. Need to implement this.
		}

		cellSpursModuleExit(spu);
	}

	return false;
}

// Wait for an external event or exit the SPURS thread group if no workloads can be scheduled
void spursSysServiceIdleHandler(spu_thread& spu, SpursKernelContext* ctxt)
{
	bool shouldExit;

	while (true)
	{
		const auto spurs = spu._ptr<CellSpurs>(0x100);
		//vm::reservation_acquire(ctxt->spurs.addr());

		// Find the number of SPUs that are idling in this SPURS instance
		u32 nIdlingSpus = 0;
		for (u32 i = 0; i < 8; i++)
		{
			if (spurs->spuIdling & (1 << i))
			{
				nIdlingSpus++;
			}
		}

		const bool allSpusIdle = nIdlingSpus == spurs->nSpus;
		const bool exitIfNoWork = spurs->flags1 & SF1_EXIT_IF_NO_WORK ? true : false;
		shouldExit = allSpusIdle && exitIfNoWork;

		// Check if any workloads can be scheduled
		bool foundReadyWorkload = false;
		if (spurs->sysSrvMessage & (1 << ctxt->spuNum))
		{
			foundReadyWorkload = true;
		}
		else
		{
			if (spurs->flags1 & SF1_32_WORKLOADS)
			{
				for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD2; i++)
				{
					u32 j = i & 0x0F;
					u16 runnable = i < CELL_SPURS_MAX_WORKLOAD ? ctxt->wklRunnable1 & (0x8000 >> j) : ctxt->wklRunnable2 & (0x8000 >> j);
					u8 priority = i < CELL_SPURS_MAX_WORKLOAD ? ctxt->priority[j] & 0x0F : ctxt->priority[j] >> 4;
					u8 maxContention = i < CELL_SPURS_MAX_WORKLOAD ? spurs->wklMaxContention[j] & 0x0F : spurs->wklMaxContention[j] >> 4;
					u8 contention = i < CELL_SPURS_MAX_WORKLOAD ? spurs->wklCurrentContention[j] & 0x0F : spurs->wklCurrentContention[j] >> 4;
					u16 wklSignal = i < CELL_SPURS_MAX_WORKLOAD ? spurs->wklSignal1.load() & (0x8000 >> j) : spurs->wklSignal2.load() & (0x8000 >> j);
					u8 wklFlag = spurs->wklFlag.flag.load() == 0u ? spurs->wklFlagReceiver == i ? 1 : 0 : 0;
					u8 readyCount = i < CELL_SPURS_MAX_WORKLOAD ? spurs->wklReadyCount1[j] : spurs->wklIdleSpuCountOrReadyCount2[j];

					if (runnable && priority > 0 && maxContention > contention)
					{
						if (wklFlag || wklSignal || readyCount > contention)
						{
							foundReadyWorkload = true;
							break;
						}
					}
				}
			}
			else
			{
				for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++)
				{
					u16 runnable = ctxt->wklRunnable1 & (0x8000 >> i);
					u16 wklSignal = spurs->wklSignal1.load() & (0x8000 >> i);
					u8 wklFlag = spurs->wklFlag.flag.load() == 0u ? spurs->wklFlagReceiver == i ? 1 : 0 : 0;
					u8 readyCount = spurs->wklReadyCount1[i] > CELL_SPURS_MAX_SPU ? CELL_SPURS_MAX_SPU : spurs->wklReadyCount1[i].load();
					u8 idleSpuCount = spurs->wklIdleSpuCountOrReadyCount2[i] > CELL_SPURS_MAX_SPU ? CELL_SPURS_MAX_SPU : spurs->wklIdleSpuCountOrReadyCount2[i].load();
					u8 requestCount = readyCount + idleSpuCount;

					if (runnable && ctxt->priority[i] != 0 && spurs->wklMaxContention[i] > spurs->wklCurrentContention[i])
					{
						if (wklFlag || wklSignal || (readyCount != 0 && requestCount > spurs->wklCurrentContention[i]))
						{
							foundReadyWorkload = true;
							break;
						}
					}
				}
			}
		}

		const bool spuIdling = spurs->spuIdling & (1 << ctxt->spuNum) ? true : false;
		if (foundReadyWorkload && shouldExit == false)
		{
			spurs->spuIdling &= ~(1 << ctxt->spuNum);
		}
		else
		{
			spurs->spuIdling |= 1 << ctxt->spuNum;
		}

		// If all SPUs are idling and the exit_if_no_work flag is set then the SPU thread group must exit. Otherwise wait for external events.
		if (spuIdling && shouldExit == false && foundReadyWorkload == false)
		{
			// The system service blocks by making a reservation and waiting on the lock line reservation lost event.
			thread_ctrl::wait_for(1000);
			continue;
		}

		//if (vm::reservation_update(vm::cast(ctxt->spurs.addr()), spu._ptr<void>(0x100), 128) && (shouldExit || foundReadyWorkload))
		{
			break;
		}
	}

	if (shouldExit)
	{
		// TODO: exit spu thread group
	}
}

// Main function for the system service
void spursSysServiceMain(spu_thread& spu, u32 /*pollStatus*/)
{
	const auto ctxt = spu._ptr<SpursKernelContext>(0x100);

	if (!ctxt->spurs.aligned())
	{
		spu_log.error("spursSysServiceMain(): invalid spurs alignment");
		spursHalt(spu);
	}

	// Initialise the system service if this is the first time its being started on this SPU
	if (ctxt->sysSrvInitialised == 0)
	{
		ctxt->sysSrvInitialised = 1;

		//vm::reservation_acquire(ctxt->spurs.addr());

		//vm::reservation_op(ctxt->spurs.ptr(&CellSpurs::wklState1).addr(), [&]()
		{
			auto spurs = ctxt->spurs.get_ptr();

			// Halt if already initialised
			if (spurs->sysSrvOnSpu & (1 << ctxt->spuNum))
			{
				spu_log.error("spursSysServiceMain(): already initialized");
				spursHalt(spu);
			}

			spurs->sysSrvOnSpu |= 1 << ctxt->spuNum;

			std::memcpy(spu._ptr<void>(0x2D80), spurs->wklState1, 128);
		}//);

		ctxt->traceBuffer = 0;
		ctxt->traceMsgCount = -1;
		spursSysServiceTraceUpdate(spu, ctxt, 1, 1, 0);
		spursSysServiceCleanupAfterSystemWorkload(spu, ctxt);

		// Trace - SERVICE: INIT
		CellSpursTracePacket pkt{};
		pkt.header.tag = CELL_SPURS_TRACE_TAG_SERVICE;
		pkt.data.service.incident = CELL_SPURS_TRACE_SERVICE_INIT;
		cellSpursModulePutTrace(&pkt, ctxt->dmaTagId);
	}

	// Trace - START: Module='SYS '
	CellSpursTracePacket pkt{};
	pkt.header.tag = CELL_SPURS_TRACE_TAG_START;
	std::memcpy(pkt.data.start._module, "SYS ", 4);
	pkt.data.start.level = 1; // Policy module
	pkt.data.start.ls = 0xA00 >> 2;
	cellSpursModulePutTrace(&pkt, ctxt->dmaTagId);

	while (true)
	{
		// Process requests for the system service
		spursSysServiceProcessRequests(spu, ctxt);

	poll:
		if (cellSpursModulePollStatus(spu, nullptr))
		{
			// Trace - SERVICE: EXIT
			CellSpursTracePacket pkt{};
			pkt.header.tag = CELL_SPURS_TRACE_TAG_SERVICE;
			pkt.data.service.incident = CELL_SPURS_TRACE_SERVICE_EXIT;
			cellSpursModulePutTrace(&pkt, ctxt->dmaTagId);

			// Trace - STOP: GUID
			pkt = {};
			pkt.header.tag = CELL_SPURS_TRACE_TAG_STOP;
			pkt.data.stop = SPURS_GUID_SYS_WKL;
			cellSpursModulePutTrace(&pkt, ctxt->dmaTagId);

			//spursDmaWaitForCompletion(spu, 1 << ctxt->dmaTagId);
			break;
		}

		// If we reach here it means that either there are more system service messages to be processed
		// or there are no workloads that can be scheduled.

		// If the SPU is not idling then process the remaining system service messages
		if (ctxt->spuIdling == 0)
		{
			continue;
		}

		// If we reach here it means that the SPU is idling

		// Trace - SERVICE: WAIT
		CellSpursTracePacket pkt{};
		pkt.header.tag = CELL_SPURS_TRACE_TAG_SERVICE;
		pkt.data.service.incident = CELL_SPURS_TRACE_SERVICE_WAIT;
		cellSpursModulePutTrace(&pkt, ctxt->dmaTagId);

		spursSysServiceIdleHandler(spu, ctxt);

		goto poll;
	}
}

// Process any requests
void spursSysServiceProcessRequests(spu_thread& spu, SpursKernelContext* ctxt)
{
	bool updateTrace = false;
	bool updateWorkload = false;
	bool terminate = false;

	//vm::reservation_op(vm::cast(ctxt->spurs.addr() + OFFSET_32(CellSpurs, wklState1)), 128, [&]()
	{
		auto spurs = ctxt->spurs.get_ptr();

		// Terminate request
		if (spurs->sysSrvMsgTerminate & (1 << ctxt->spuNum))
		{
			spurs->sysSrvOnSpu &= ~(1 << ctxt->spuNum);
			terminate = true;
		}

		// Update workload message
		if (spurs->sysSrvMsgUpdateWorkload & (1 << ctxt->spuNum))
		{
			spurs->sysSrvMsgUpdateWorkload &= ~(1 << ctxt->spuNum);
			updateWorkload = true;
		}

		// Update trace message
		if (spurs->sysSrvTrace.load().sysSrvMsgUpdateTrace & (1 << ctxt->spuNum))
		{
			updateTrace = true;
		}

		std::memcpy(spu._ptr<void>(0x2D80), spurs->wklState1, 128);
	}//);

	// Process update workload message
	if (updateWorkload)
	{
		spursSysServiceActivateWorkload(spu, ctxt);
	}

	// Process update trace message
	if (updateTrace)
	{
		spursSysServiceTraceUpdate(spu, ctxt, 1, 0, 0);
	}

	// Process terminate request
	if (terminate)
	{
		// TODO: Rest of the terminate processing
	}
}

// Activate a workload
void spursSysServiceActivateWorkload(spu_thread& spu, SpursKernelContext* ctxt)
{
	const auto spurs = spu._ptr<CellSpurs>(0x100);
	std::memcpy(spu._ptr<void>(0x30000), ctxt->spurs->wklInfo1, 0x200);
	if (spurs->flags1 & SF1_32_WORKLOADS)
	{
		std::memcpy(spu._ptr<void>(0x30200), ctxt->spurs->wklInfo2, 0x200);
	}

	u32 wklShutdownBitSet = 0;
	ctxt->wklRunnable1 = 0;
	ctxt->wklRunnable2 = 0;
	for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++)
	{
		const auto wklInfo1 = spu._ptr<CellSpurs::WorkloadInfo>(0x30000);

		// Copy the priority of the workload for this SPU and its unique id to the LS
		ctxt->priority[i] = wklInfo1[i].priority[ctxt->spuNum] == 0 ? 0 : 0x10 - wklInfo1[i].priority[ctxt->spuNum];
		ctxt->wklUniqueId[i] = wklInfo1[i].uniqueId;

		if (spurs->flags1 & SF1_32_WORKLOADS)
		{
			const auto wklInfo2 = spu._ptr<CellSpurs::WorkloadInfo>(0x30200);

			// Copy the priority of the workload for this SPU to the LS
			if (wklInfo2[i].priority[ctxt->spuNum])
			{
				ctxt->priority[i] |= (0x10 - wklInfo2[i].priority[ctxt->spuNum]) << 4;
			}
		}
	}

	//vm::reservation_op(ctxt->spurs.ptr(&CellSpurs::wklState1).addr(), 128, [&]()
	{
		auto spurs = ctxt->spurs.get_ptr();

		for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++)
		{
			// Update workload status and runnable flag based on the workload state
			auto wklStatus = spurs->wklStatus1[i];
			if (spurs->wklState1[i] == SPURS_WKL_STATE_RUNNABLE)
			{
				spurs->wklStatus1[i] |= 1 << ctxt->spuNum;
				ctxt->wklRunnable1 |= 0x8000 >> i;
			}
			else
			{
				spurs->wklStatus1[i] &= ~(1 << ctxt->spuNum);
			}

			// If the workload is shutting down and if this is the last SPU from which it is being removed then
			// add it to the shutdown bit set
			if (spurs->wklState1[i] == SPURS_WKL_STATE_SHUTTING_DOWN)
			{
				if (((wklStatus & (1 << ctxt->spuNum)) != 0) && (spurs->wklStatus1[i] == 0))
				{
					spurs->wklState1[i] = SPURS_WKL_STATE_REMOVABLE;
					wklShutdownBitSet |= 0x80000000u >> i;
				}
			}

			if (spurs->flags1 & SF1_32_WORKLOADS)
			{
				// Update workload status and runnable flag based on the workload state
				wklStatus = spurs->wklStatus2[i];
				if (spurs->wklState2[i] == SPURS_WKL_STATE_RUNNABLE)
				{
					spurs->wklStatus2[i] |= 1 << ctxt->spuNum;
					ctxt->wklRunnable2 |= 0x8000 >> i;
				}
				else
				{
					spurs->wklStatus2[i] &= ~(1 << ctxt->spuNum);
				}

				// If the workload is shutting down and if this is the last SPU from which it is being removed then
				// add it to the shutdown bit set
				if (spurs->wklState2[i] == SPURS_WKL_STATE_SHUTTING_DOWN)
				{
					if (((wklStatus & (1 << ctxt->spuNum)) != 0) && (spurs->wklStatus2[i] == 0))
					{
						spurs->wklState2[i] = SPURS_WKL_STATE_REMOVABLE;
						wklShutdownBitSet |= 0x8000 >> i;
					}
				}
			}
		}

		std::memcpy(spu._ptr<void>(0x2D80), spurs->wklState1, 128);
	}//);

	if (wklShutdownBitSet)
	{
		spursSysServiceUpdateShutdownCompletionEvents(spu, ctxt, wklShutdownBitSet);
	}
}

// Update shutdown completion events
void spursSysServiceUpdateShutdownCompletionEvents(spu_thread& spu, SpursKernelContext* ctxt, u32 wklShutdownBitSet)
{
	// Mark the workloads in wklShutdownBitSet as completed and also generate a bit set of the completed
	// workloads that have a shutdown completion hook registered
	u32 wklNotifyBitSet;
	[[maybe_unused]] u8 spuPort;
	//vm::reservation_op(ctxt->spurs.ptr(&CellSpurs::wklState1).addr(), 128, [&]()
	{
		auto spurs = ctxt->spurs.get_ptr();

		wklNotifyBitSet = 0;
		spuPort = spurs->spuPort;
		for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++)
		{
			if (wklShutdownBitSet & (0x80000000u >> i))
			{
				spurs->wklEvent1[i] |= 0x01;
				if (spurs->wklEvent1[i] & 0x02 || spurs->wklEvent1[i] & 0x10)
				{
					wklNotifyBitSet |= 0x80000000u >> i;
				}
			}

			if (wklShutdownBitSet & (0x8000 >> i))
			{
				spurs->wklEvent2[i] |= 0x01;
				if (spurs->wklEvent2[i] & 0x02 || spurs->wklEvent2[i] & 0x10)
				{
					wklNotifyBitSet |= 0x8000 >> i;
				}
			}
		}

		std::memcpy(spu._ptr<void>(0x2D80), spurs->wklState1, 128);
	}//);

	if (wklNotifyBitSet)
	{
		// TODO: sys_spu_thread_send_event(spuPort, 0, wklNotifyMask);
	}
}

// Update the trace count for this SPU
void spursSysServiceTraceSaveCount(spu_thread& /*spu*/, SpursKernelContext* ctxt)
{
	if (ctxt->traceBuffer)
	{
		auto traceInfo = vm::ptr<CellSpursTraceInfo>::make(vm::cast(ctxt->traceBuffer - (ctxt->spurs->traceStartIndex[ctxt->spuNum] << 4)));
		traceInfo->count[ctxt->spuNum] = ctxt->traceMsgCount;
	}
}

// Update trace control
void spursSysServiceTraceUpdate(spu_thread& spu, SpursKernelContext* ctxt, u32 arg2, u32 arg3, u32 forceNotify)
{
	bool notify;

	u8 sysSrvMsgUpdateTrace;
	//vm::reservation_op(ctxt->spurs.ptr(&CellSpurs::wklState1).addr(), 128, [&]()
	{
		auto spurs = ctxt->spurs.get_ptr();
		auto& trace = spurs->sysSrvTrace.raw();

		sysSrvMsgUpdateTrace = trace.sysSrvMsgUpdateTrace;
		trace.sysSrvMsgUpdateTrace &= ~(1 << ctxt->spuNum);
		trace.sysSrvTraceInitialised &= ~(1 << ctxt->spuNum);
		trace.sysSrvTraceInitialised |= arg2 << ctxt->spuNum;

		notify = false;
		if (((sysSrvMsgUpdateTrace & (1 << ctxt->spuNum)) != 0) && (spurs->sysSrvTrace.load().sysSrvMsgUpdateTrace == 0) && (spurs->sysSrvTrace.load().sysSrvNotifyUpdateTraceComplete != 0))
		{
			trace.sysSrvNotifyUpdateTraceComplete = 0;
			notify = true;
		}

		if (forceNotify && spurs->sysSrvTrace.load().sysSrvNotifyUpdateTraceComplete != 0)
		{
			trace.sysSrvNotifyUpdateTraceComplete = 0;
			notify = true;
		}

		std::memcpy(spu._ptr<void>(0x2D80), spurs->wklState1, 128);
	}//);

	// Get trace parameters from CellSpurs and store them in the LS
	if (((sysSrvMsgUpdateTrace & (1 << ctxt->spuNum)) != 0) || (arg3 != 0))
	{
		//vm::reservation_acquire(ctxt->spurs.ptr(&CellSpurs::traceBuffer).addr());
		auto spurs = spu._ptr<CellSpurs>(0x80 - offset32(&CellSpurs::traceBuffer));

		if (ctxt->traceMsgCount != 0xffu || spurs->traceBuffer.addr() == 0u)
		{
			spursSysServiceTraceSaveCount(spu, ctxt);
		}
		else
		{
			const auto traceBuffer = spu._ptr<CellSpursTraceInfo>(0x2C00);
			std::memcpy(traceBuffer, vm::base(vm::cast(spurs->traceBuffer.addr()) & -0x4), 0x80);
			ctxt->traceMsgCount = traceBuffer->count[ctxt->spuNum];
		}

		ctxt->traceBuffer = spurs->traceBuffer.addr() + (spurs->traceStartIndex[ctxt->spuNum] << 4);
		ctxt->traceMaxCount = spurs->traceStartIndex[1] - spurs->traceStartIndex[0];
		if (ctxt->traceBuffer == 0u)
		{
			ctxt->traceMsgCount = 0u;
		}
	}

	if (notify)
	{
		auto spurs = spu._ptr<CellSpurs>(0x2D80 - offset32(&CellSpurs::wklState1));
		sys_spu_thread_send_event(spu, spurs->spuPort, 2, 0);
	}
}

// Restore state after executing the system workload
void spursSysServiceCleanupAfterSystemWorkload(spu_thread& spu, SpursKernelContext* ctxt)
{
	u8 wklId;

	bool do_return = false;

	//vm::reservation_op(ctxt->spurs.ptr(&CellSpurs::wklState1).addr(), 128, [&]()
	{
		auto spurs = ctxt->spurs.get_ptr();

		if (spurs->sysSrvPreemptWklId[ctxt->spuNum] == 0xFF)
		{
			do_return = true;
			return;
		}

		wklId = spurs->sysSrvPreemptWklId[ctxt->spuNum];
		spurs->sysSrvPreemptWklId[ctxt->spuNum] = 0xFF;

		std::memcpy(spu._ptr<void>(0x2D80), spurs->wklState1, 128);
	}//);

	if (do_return) return;

	spursSysServiceActivateWorkload(spu, ctxt);

	//vm::reservation_op(vm::cast(ctxt->spurs.addr()), 128, [&]()
	{
		auto spurs = ctxt->spurs.get_ptr();

		if (wklId >= CELL_SPURS_MAX_WORKLOAD)
		{
			spurs->wklCurrentContention[wklId & 0x0F] -= 0x10;
			spurs->wklReadyCount1[wklId & 0x0F].raw() -= 1;
		}
		else
		{
			spurs->wklCurrentContention[wklId & 0x0F] -= 0x01;
			spurs->wklIdleSpuCountOrReadyCount2[wklId & 0x0F].raw() -= 1;
		}

		std::memcpy(spu._ptr<void>(0x100), spurs, 128);
	}//);

	// Set the current workload id to the id of the pre-empted workload since cellSpursModulePutTrace
	// uses the current worload id to determine the workload to which the trace belongs
	auto wklIdSaved = ctxt->wklCurrentId;
	ctxt->wklCurrentId = wklId;

	// Trace - STOP: GUID
	CellSpursTracePacket pkt{};
	pkt.header.tag = CELL_SPURS_TRACE_TAG_STOP;
	pkt.data.stop = SPURS_GUID_SYS_WKL;
	cellSpursModulePutTrace(&pkt, ctxt->dmaTagId);

	ctxt->wklCurrentId = wklIdSaved;
}

//----------------------------------------------------------------------------
// SPURS taskset policy module functions
//----------------------------------------------------------------------------

enum SpursTasksetRequest
{
	SPURS_TASKSET_REQUEST_POLL_SIGNAL = -1,
	SPURS_TASKSET_REQUEST_DESTROY_TASK = 0,
	SPURS_TASKSET_REQUEST_YIELD_TASK = 1,
	SPURS_TASKSET_REQUEST_WAIT_SIGNAL = 2,
	SPURS_TASKSET_REQUEST_POLL = 3,
	SPURS_TASKSET_REQUEST_WAIT_WKL_FLAG = 4,
	SPURS_TASKSET_REQUEST_SELECT_TASK = 5,
	SPURS_TASKSET_REQUEST_RECV_WKL_FLAG = 6,
};

// Taskset PM entry point
bool spursTasksetEntry(spu_thread& spu)
{
	auto ctxt = spu._ptr<SpursTasksetContext>(0x2700);
	auto kernelCtxt = spu._ptr<SpursKernelContext>(spu.gpr[3]._u32[3]);

	auto arg = spu.gpr[4]._u64[1];
	auto pollStatus = spu.gpr[5]._u32[3];

	// Initialise memory and save args
	memset(ctxt, 0, sizeof(*ctxt));
	ctxt->taskset.set(arg);
	memcpy(ctxt->moduleId, "SPURSTASK MODULE", sizeof(ctxt->moduleId));
	ctxt->kernelMgmtAddr = spu.gpr[3]._u32[3];
	ctxt->syscallAddr = CELL_SPURS_TASKSET_PM_SYSCALL_ADDR;
	ctxt->spuNum = kernelCtxt->spuNum;
	ctxt->dmaTagId = kernelCtxt->dmaTagId;
	ctxt->taskId = 0xFFFFFFFF;

	// Register SPURS takset policy module HLE functions
	//spu.UnregisterHleFunctions(CELL_SPURS_TASKSET_PM_ENTRY_ADDR, 0x40000/*LS_BOTTOM*/);
	//spu.RegisterHleFunction(CELL_SPURS_TASKSET_PM_ENTRY_ADDR, spursTasksetEntry);
	//spu.RegisterHleFunction(ctxt->syscallAddr, spursTasksetSyscallEntry);

	{
		// Initialise the taskset policy module
		spursTasksetInit(spu, pollStatus);

		// Dispatch
		spursTasksetDispatch(spu);
	}

	return false;
}

// Entry point into the Taskset PM for task syscalls
bool spursTasksetSyscallEntry(spu_thread& spu)
{
	auto ctxt = spu._ptr<SpursTasksetContext>(0x2700);

	{
		// Save task context
		ctxt->savedContextLr = spu.gpr[0];
		ctxt->savedContextSp = spu.gpr[1];
		for (auto i = 0; i < 48; i++)
		{
			ctxt->savedContextR80ToR127[i] = spu.gpr[80 + i];
		}

		// Handle the syscall
		spu.gpr[3]._u32[3] = spursTasksetProcessSyscall(spu, spu.gpr[3]._u32[3], spu.gpr[4]._u32[3]);

		// Resume the previously executing task if the syscall did not cause a context switch
		fmt::throw_exception("Broken (TODO)");
		//if (spu.m_is_branch == false) {
		//    spursTasksetResumeTask(spu);
		//}
	}

	return false;
}

// Resume a task
void spursTasksetResumeTask(spu_thread& spu)
{
	auto ctxt = spu._ptr<SpursTasksetContext>(0x2700);

	// Restore task context
	spu.gpr[0] = ctxt->savedContextLr;
	spu.gpr[1] = ctxt->savedContextSp;
	for (auto i = 0; i < 48; i++)
	{
		spu.gpr[80 + i] = ctxt->savedContextR80ToR127[i];
	}

	spu.pc = spu.gpr[0]._u32[3];
}

// Start a task
void spursTasksetStartTask(spu_thread& spu, CellSpursTaskArgument& taskArgs)
{
	auto ctxt = spu._ptr<SpursTasksetContext>(0x2700);
	auto taskset = spu._ptr<CellSpursTaskset>(0x2700);

	spu.gpr[2].clear();
	spu.gpr[3] = v128::from64r(taskArgs._u64[0], taskArgs._u64[1]);
	spu.gpr[4]._u64[1] = taskset->args;
	spu.gpr[4]._u64[0] = taskset->spurs.addr();
	for (auto i = 5; i < 128; i++)
	{
		spu.gpr[i].clear();
	}

	spu.pc = ctxt->savedContextLr.value()._u32[3];
}

// Process a request and update the state of the taskset
s32 spursTasksetProcessRequest(spu_thread& spu, s32 request, u32* taskId, u32* isWaiting)
{
	auto kernelCtxt = spu._ptr<SpursKernelContext>(0x100);
	auto ctxt = spu._ptr<SpursTasksetContext>(0x2700);

	s32 rc = CELL_OK;
	s32 numNewlyReadyTasks = 0;

	//vm::reservation_op(vm::cast(ctxt->taskset.addr()), 128, [&]()
	{
		auto taskset = ctxt->taskset;
		v128 waiting = vm::_ref<v128>(ctxt->taskset.addr() + ::offset32(&CellSpursTaskset::waiting));
		v128 running = vm::_ref<v128>(ctxt->taskset.addr() + ::offset32(&CellSpursTaskset::running));
		v128 ready = vm::_ref<v128>(ctxt->taskset.addr() + ::offset32(&CellSpursTaskset::ready));
		v128 pready = vm::_ref<v128>(ctxt->taskset.addr() + ::offset32(&CellSpursTaskset::pending_ready));
		v128 enabled = vm::_ref<v128>(ctxt->taskset.addr() + ::offset32(&CellSpursTaskset::enabled));
		v128 signalled = vm::_ref<v128>(ctxt->taskset.addr() + ::offset32(&CellSpursTaskset::signalled));

		// Verify taskset state is valid
		if ((waiting & running) != v128{} || (ready & pready) != v128{} ||
			(gv_andn(enabled, running | ready | pready | signalled | waiting) != v128{}))
		{
			spu_log.error("Invalid taskset state");
			spursHalt(spu);
		}

		// Find the number of tasks that have become ready since the last iteration
		{
			v128 newlyReadyTasks = gv_andn(ready, signalled | pready);

			numNewlyReadyTasks = utils::popcnt128(newlyReadyTasks._u);
		}

		v128 readyButNotRunning;
		u8   selectedTaskId;
		v128 signalled0 = (signalled & (ready | pready));
		v128 ready0 = (signalled | ready | pready);

		u128 ctxtTaskIdMask = u128{1} << +(~ctxt->taskId & 127);

		switch (request)
		{
		case SPURS_TASKSET_REQUEST_POLL_SIGNAL:
		{
			rc = signalled0._u & ctxtTaskIdMask ? 1 : 0;
			signalled0._u &= ~ctxtTaskIdMask;
			break;
		}
		case SPURS_TASKSET_REQUEST_DESTROY_TASK:
		{
			numNewlyReadyTasks--;
			running._u &= ~ctxtTaskIdMask;
			enabled._u &= ~ctxtTaskIdMask;
			signalled0._u &= ~ctxtTaskIdMask;
			ready0._u &= ~ctxtTaskIdMask;
			break;
		}
		case SPURS_TASKSET_REQUEST_YIELD_TASK:
		{
			running._u &= ~ctxtTaskIdMask;
			waiting._u |= ctxtTaskIdMask;
			break;
		}
		case SPURS_TASKSET_REQUEST_WAIT_SIGNAL:
		{
			if (!(signalled0._u & ctxtTaskIdMask))
			{
				numNewlyReadyTasks--;
				running._u &= ~ctxtTaskIdMask;
				waiting._u |= ctxtTaskIdMask;
				signalled0._u &= ~ctxtTaskIdMask;
				ready0._u &= ~ctxtTaskIdMask;
			}
			break;
		}
		case SPURS_TASKSET_REQUEST_POLL:
		{
			readyButNotRunning = gv_andn(running, ready0);
			if (taskset->wkl_flag_wait_task < CELL_SPURS_MAX_TASK)
			{
				readyButNotRunning._u &= ~(u128{1} << (~taskset->wkl_flag_wait_task & 127));
			}

			rc = readyButNotRunning._u ? 1 : 0;
			break;
		}
		case SPURS_TASKSET_REQUEST_WAIT_WKL_FLAG:
		{
			if (taskset->wkl_flag_wait_task == 0x81)
			{
				// A workload flag is already pending so consume it
				taskset->wkl_flag_wait_task = 0x80;
				rc = 0;
			}
			else if (taskset->wkl_flag_wait_task == 0x80)
			{
				// No tasks are waiting for the workload flag. Mark this task as waiting for the workload flag.
				taskset->wkl_flag_wait_task = ctxt->taskId;
				running._u &= ~ctxtTaskIdMask;
				waiting._u |= ctxtTaskIdMask;
				rc = 1;
				numNewlyReadyTasks--;
			}
			else
			{
				// Another task is already waiting for the workload signal
				rc = CELL_SPURS_TASK_ERROR_BUSY;
			}
			break;
		}
		case SPURS_TASKSET_REQUEST_SELECT_TASK:
		{
			readyButNotRunning = gv_andn(running, ready0);
			if (taskset->wkl_flag_wait_task < CELL_SPURS_MAX_TASK)
			{
				readyButNotRunning._u &= ~(u128{1} << (~taskset->wkl_flag_wait_task & 127));
			}

			// Select a task from the readyButNotRunning set to run. Start from the task after the last scheduled task to ensure fairness.
			for (selectedTaskId = taskset->last_scheduled_task + 1; selectedTaskId < 128; selectedTaskId++)
			{
				if (readyButNotRunning._u & (u128{1} << (~selectedTaskId & 127)))
				{
					break;
				}
			}

			if (selectedTaskId == 128)
			{
				for (selectedTaskId = 0; selectedTaskId < taskset->last_scheduled_task + 1; selectedTaskId++)
				{
					if (readyButNotRunning._u & (u128{1} << (~selectedTaskId & 127)))
					{
						break;
					}
				}

				if (selectedTaskId == taskset->last_scheduled_task + 1)
				{
					selectedTaskId = CELL_SPURS_MAX_TASK;
				}
			}

			*taskId = selectedTaskId;

			if (selectedTaskId != CELL_SPURS_MAX_TASK)
			{
				const u128 selectedTaskIdMask = u128{1} << (~selectedTaskId & 127);

				*isWaiting = waiting._u & selectedTaskIdMask ? 1 : 0;
				taskset->last_scheduled_task = selectedTaskId;
				running._u |= selectedTaskIdMask;
				waiting._u &= ~selectedTaskIdMask;
			}
			else
			{
				*isWaiting = waiting._u & (u128{1} << 127) ? 1 : 0;
			}

			break;
		}
		case SPURS_TASKSET_REQUEST_RECV_WKL_FLAG:
		{
			if (taskset->wkl_flag_wait_task < CELL_SPURS_MAX_TASK)
			{
				// There is a task waiting for the workload flag
				taskset->wkl_flag_wait_task = 0x80;
				rc = 1;
				numNewlyReadyTasks++;
			}
			else
			{
				// No tasks are waiting for the workload flag
				taskset->wkl_flag_wait_task = 0x81;
				rc = 0;
			}
			break;
		}
		default:
			spu_log.error("Unknown taskset request");
			spursHalt(spu);
		}

		// vm::_ref<v128>(ctxt->taskset.addr() + ::offset32(&CellSpursTaskset::waiting)) = waiting;
		// vm::_ref<v128>(ctxt->taskset.addr() + ::offset32(&CellSpursTaskset::running)) = running;
		// vm::_ref<v128>(ctxt->taskset.addr() + ::offset32(&CellSpursTaskset::ready)) = ready;
		// vm::_ref<v128>(ctxt->taskset.addr() + ::offset32(&CellSpursTaskset::pending_ready)) = v128{};
		// vm::_ref<v128>(ctxt->taskset.addr() + ::offset32(&CellSpursTaskset::enabled)) = enabled;
		// vm::_ref<v128>(ctxt->taskset.addr() + ::offset32(&CellSpursTaskset::signalled)) = signalled;

		std::memcpy(spu._ptr<void>(0x2700), spu._ptr<void>(0x100), 128); // Copy data
	}//);

	// Increment the ready count of the workload by the number of tasks that have become ready
	if (numNewlyReadyTasks)
	{
		auto spurs = kernelCtxt->spurs;

		vm::light_op(spurs->readyCount(kernelCtxt->wklCurrentId), [&](atomic_t<u8>& val)
		{
			val.fetch_op([&](u8& val)
			{
				const s32 _new = val + numNewlyReadyTasks;
				val = static_cast<u8>(std::clamp<s32>(_new, 0, 0xFF));
			});
		});
	}

	return rc;
}

// Process pollStatus received from the SPURS kernel
void spursTasksetProcessPollStatus(spu_thread& spu, u32 pollStatus)
{
	if (pollStatus & CELL_SPURS_MODULE_POLL_STATUS_FLAG)
	{
		spursTasksetProcessRequest(spu, SPURS_TASKSET_REQUEST_RECV_WKL_FLAG, nullptr, nullptr);
	}
}

// Check execution rights
bool spursTasksetPollStatus(spu_thread& spu)
{
	u32 pollStatus;

	if (cellSpursModulePollStatus(spu, &pollStatus))
	{
		return true;
	}

	spursTasksetProcessPollStatus(spu, pollStatus);
	return false;
}

// Exit the Taskset PM
void spursTasksetExit(spu_thread& spu)
{
	auto ctxt = spu._ptr<SpursTasksetContext>(0x2700);

	// Trace - STOP
	CellSpursTracePacket pkt{};
	pkt.header.tag = 0x54; // Its not clear what this tag means exactly but it seems similar to CELL_SPURS_TRACE_TAG_STOP
	pkt.data.stop = SPURS_GUID_TASKSET_PM;
	cellSpursModulePutTrace(&pkt, ctxt->dmaTagId);

	// Not sure why this check exists. Perhaps to check for memory corruption.
	if (memcmp(ctxt->moduleId, "SPURSTASK MODULE", 16) != 0)
	{
		spu_log.error("spursTasksetExit(): memory corruption");
		spursHalt(spu);
	}

	cellSpursModuleExit(spu);
}

// Invoked when a task exits
void spursTasksetOnTaskExit(spu_thread& spu, u64 addr, u32 taskId, s32 exitCode, u64 args)
{
	auto ctxt = spu._ptr<SpursTasksetContext>(0x2700);

	std::memcpy(spu._ptr<void>(0x10000), vm::base(addr & -0x80), (addr & 0x7F) << 11);

	spu.gpr[3]._u64[1] = ctxt->taskset.addr();
	spu.gpr[4]._u32[3] = taskId;
	spu.gpr[5]._u32[3] = exitCode;
	spu.gpr[6]._u64[1] = args;
	spu.fast_call(0x10000);
}

// Save the context of a task
s32 spursTasketSaveTaskContext(spu_thread& spu)
{
	auto ctxt = spu._ptr<SpursTasksetContext>(0x2700);
	auto taskInfo = spu._ptr<CellSpursTaskset::TaskInfo>(0x2780);

	//spursDmaWaitForCompletion(spu, 0xFFFFFFFF);

	if (taskInfo->context_save_storage_and_alloc_ls_blocks == 0u)
	{
		return CELL_SPURS_TASK_ERROR_STAT;
	}

	u32 allocLsBlocks = static_cast<u32>(taskInfo->context_save_storage_and_alloc_ls_blocks & 0x7F);
	v128 ls_pattern = v128::from64r(taskInfo->ls_pattern._u64[0], taskInfo->ls_pattern._u64[1]);

	const u32 lsBlocks = utils::popcnt128(ls_pattern._u);

	if (lsBlocks > allocLsBlocks)
	{
		return CELL_SPURS_TASK_ERROR_STAT;
	}

	// Make sure the stack is area is specified in the ls pattern
	for (auto i = (ctxt->savedContextSp.value()._u32[3]) >> 11; i < 128; i++)
	{
		if (!(ls_pattern._u & (u128{1} << (i ^ 127))))
		{
			return CELL_SPURS_TASK_ERROR_STAT;
		}
	}

	// Get the processor context
	v128 r;
	spu.fpscr.Read(r);
	ctxt->savedContextFpscr = r;
	ctxt->savedSpuWriteEventMask = static_cast<u32>(spu.get_ch_value(SPU_RdEventMask));
	ctxt->savedWriteTagGroupQueryMask = static_cast<u32>(spu.get_ch_value(MFC_RdTagMask));

	// Store the processor context
	const u32 contextSaveStorage = vm::cast(taskInfo->context_save_storage_and_alloc_ls_blocks & -0x80);
	std::memcpy(vm::base(contextSaveStorage), spu._ptr<void>(0x2C80), 0x380);

	// Save LS context
	for (auto i = 6; i < 128; i++)
	{
		if (ls_pattern._u & (u128{1} << (i ^ 127)))
		{
			// TODO: Combine DMA requests for consecutive blocks into a single request
			std::memcpy(vm::base(contextSaveStorage + 0x400 + ((i - 6) << 11)), spu._ptr<void>(CELL_SPURS_TASK_TOP + ((i - 6) << 11)), 0x800);
		}
	}

	//spursDmaWaitForCompletion(spu, 1 << ctxt->dmaTagId);
	return CELL_OK;
}

// Taskset dispatcher
void spursTasksetDispatch(spu_thread& spu)
{
	const auto ctxt = spu._ptr<SpursTasksetContext>(0x2700);
	const auto taskset = spu._ptr<CellSpursTaskset>(0x2700);

	u32 taskId;
	u32 isWaiting;
	spursTasksetProcessRequest(spu, SPURS_TASKSET_REQUEST_SELECT_TASK, &taskId, &isWaiting);
	if (taskId >= CELL_SPURS_MAX_TASK)
	{
		spursTasksetExit(spu);
		return;
	}

	ctxt->taskId = taskId;

	// DMA in the task info for the selected task
	const auto taskInfo = spu._ptr<CellSpursTaskset::TaskInfo>(0x2780);
	std::memcpy(taskInfo, &ctxt->taskset->task_info[taskId], sizeof(CellSpursTaskset::TaskInfo));
	auto elfAddr = taskInfo->elf.addr().value();
	taskInfo->elf.set(taskInfo->elf.addr() & 0xFFFFFFFFFFFFFFF8);

	// Trace - Task: Incident=dispatch
	CellSpursTracePacket pkt{};
	pkt.header.tag = CELL_SPURS_TRACE_TAG_TASK;
	pkt.data.task.incident = CELL_SPURS_TRACE_TASK_DISPATCH;
	pkt.data.task.taskId = taskId;
	cellSpursModulePutTrace(&pkt, CELL_SPURS_KERNEL_DMA_TAG_ID);

	if (isWaiting == 0)
	{
		// If we reach here it means that the task is being started and not being resumed
		std::memset(spu._ptr<void>(CELL_SPURS_TASK_TOP), 0, CELL_SPURS_TASK_BOTTOM - CELL_SPURS_TASK_TOP);
		ctxt->guidAddr = CELL_SPURS_TASK_TOP;

		u32 entryPoint;
		u32 lowestLoadAddr;
		if (spursTasksetLoadElf(spu, &entryPoint, &lowestLoadAddr, taskInfo->elf.addr(), false) != CELL_OK)
		{
			spu_log.error("spursTaskLoadElf() failed");
			spursHalt(spu);
		}

		//spursDmaWaitForCompletion(spu, 1 << ctxt->dmaTagId);

		ctxt->savedContextLr = v128::from32r(entryPoint);
		ctxt->guidAddr = lowestLoadAddr;
		ctxt->tasksetMgmtAddr = 0x2700;
		ctxt->x2FC0 = 0;
		ctxt->taskExitCode = isWaiting;
		ctxt->x2FD4 = elfAddr & 5; // TODO: Figure this out

		if ((elfAddr & 5) == 1)
		{
			std::memcpy(spu._ptr<void>(0x2FC0), &vm::_ptr<CellSpursTaskset2>(vm::cast(ctxt->taskset.addr()))->task_exit_code[taskId], 0x10);
		}

		// Trace - GUID
		pkt = {};
		pkt.header.tag = CELL_SPURS_TRACE_TAG_GUID;
		pkt.data.guid = 0; // TODO: Put GUID of taskId here
		cellSpursModulePutTrace(&pkt, 0x1F);

		if (elfAddr & 2)
		{
			// TODO: Figure this out
			spu_runtime::g_escape(&spu);
		}

		spursTasksetStartTask(spu, taskInfo->args);
	}
	else
	{
		if (taskset->enable_clear_ls)
		{
			std::memset(spu._ptr<void>(CELL_SPURS_TASK_TOP), 0, CELL_SPURS_TASK_BOTTOM - CELL_SPURS_TASK_TOP);
		}

		// If the entire LS is saved then there is no need to load the ELF as it will be be saved in the context save area as well
		v128 ls_pattern = v128::from64r(taskInfo->ls_pattern._u64[0], taskInfo->ls_pattern._u64[1]);
		if (ls_pattern != v128::from64r(0x03FFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull))
		{
			// Load the ELF
			u32 entryPoint;
			if (spursTasksetLoadElf(spu, &entryPoint, nullptr, taskInfo->elf.addr(), true) != CELL_OK)
			{
				spu_log.error("spursTasksetLoadElf() failed");
				spursHalt(spu);
			}
		}

		// Load saved context from main memory to LS
		const u32 contextSaveStorage = vm::cast(taskInfo->context_save_storage_and_alloc_ls_blocks & -0x80);
		std::memcpy(spu._ptr<void>(0x2C80), vm::base(contextSaveStorage), 0x380);
		for (auto i = 6; i < 128; i++)
		{
			if (ls_pattern._u & (u128{1} << (i ^ 127)))
			{
				// TODO: Combine DMA requests for consecutive blocks into a single request
				std::memcpy(spu._ptr<void>(CELL_SPURS_TASK_TOP + ((i - 6) << 11)), vm::base(contextSaveStorage + 0x400 + ((i - 6) << 11)), 0x800);
			}
		}

		//spursDmaWaitForCompletion(spu, 1 << ctxt->dmaTagId);

		// Restore saved registers
		spu.fpscr.Write(ctxt->savedContextFpscr.value());
		spu.set_ch_value(MFC_WrTagMask, ctxt->savedWriteTagGroupQueryMask);
		spu.set_ch_value(SPU_WrEventMask, ctxt->savedSpuWriteEventMask);

		// Trace - GUID
		pkt = {};
		pkt.header.tag = CELL_SPURS_TRACE_TAG_GUID;
		pkt.data.guid = 0; // TODO: Put GUID of taskId here
		cellSpursModulePutTrace(&pkt, 0x1F);

		if (elfAddr & 2)
		{
			// TODO: Figure this out
			spu_runtime::g_escape(&spu);
		}

		spu.gpr[3].clear();
		spursTasksetResumeTask(spu);
	}
}

// Process a syscall request
s32 spursTasksetProcessSyscall(spu_thread& spu, u32 syscallNum, u32 args)
{
	auto ctxt = spu._ptr<SpursTasksetContext>(0x2700);
	auto taskset = spu._ptr<CellSpursTaskset>(0x2700);

	// If the 0x10 bit is set in syscallNum then its the 2nd version of the
	// syscall (e.g. cellSpursYield2 instead of cellSpursYield) and so don't wait
	// for DMA completion
	if ((syscallNum & 0x10) == 0)
	{
		//spursDmaWaitForCompletion(spu, 0xFFFFFFFF);
	}

	s32 rc = 0;
	u32 incident = 0;
	switch (syscallNum & 0x0F)
	{
	case CELL_SPURS_TASK_SYSCALL_EXIT:
		if (ctxt->x2FD4 == 4u || (ctxt->x2FC0 & 0xffffffffu) != 0u)
		{ // TODO: Figure this out
			if (ctxt->x2FD4 != 4u)
			{
				spursTasksetProcessRequest(spu, SPURS_TASKSET_REQUEST_DESTROY_TASK, nullptr, nullptr);
			}

			const u64 addr = ctxt->x2FD4 == 4u ? +taskset->x78 : +ctxt->x2FC0;
			const u64 args = ctxt->x2FD4 == 4u ? 0 : +ctxt->x2FC8;
			spursTasksetOnTaskExit(spu, addr, ctxt->taskId, ctxt->taskExitCode, args);
		}

		incident = CELL_SPURS_TRACE_TASK_EXIT;
		break;
	case CELL_SPURS_TASK_SYSCALL_YIELD:
		if (spursTasksetPollStatus(spu) || spursTasksetProcessRequest(spu, SPURS_TASKSET_REQUEST_POLL, nullptr, nullptr))
		{
			// If we reach here then it means that either another task can be scheduled or another workload can be scheduled
			// Save the context of the current task
			rc = spursTasketSaveTaskContext(spu);
			if (rc == CELL_OK)
			{
				spursTasksetProcessRequest(spu, SPURS_TASKSET_REQUEST_YIELD_TASK, nullptr, nullptr);
				incident = CELL_SPURS_TRACE_TASK_YIELD;
			}
		}
		break;
	case CELL_SPURS_TASK_SYSCALL_WAIT_SIGNAL:
		if (spursTasksetProcessRequest(spu, SPURS_TASKSET_REQUEST_POLL_SIGNAL, nullptr, nullptr) == 0)
		{
			rc = spursTasketSaveTaskContext(spu);
			if (rc == CELL_OK)
			{
				if (spursTasksetProcessRequest(spu, SPURS_TASKSET_REQUEST_WAIT_SIGNAL, nullptr, nullptr) == 0)
				{
					incident = CELL_SPURS_TRACE_TASK_WAIT;
				}
			}
		}
		break;
	case CELL_SPURS_TASK_SYSCALL_POLL:
		rc = spursTasksetPollStatus(spu) ? CELL_SPURS_TASK_POLL_FOUND_WORKLOAD : 0;
		rc |= spursTasksetProcessRequest(spu, SPURS_TASKSET_REQUEST_POLL, nullptr, nullptr) ? CELL_SPURS_TASK_POLL_FOUND_TASK : 0;
		break;
	case CELL_SPURS_TASK_SYSCALL_RECV_WKL_FLAG:
		if (args == 0)
		{ // TODO: Figure this out
			spu_log.error("args == 0");
			//spursHalt(spu);
		}

		if (spursTasksetPollStatus(spu) || spursTasksetProcessRequest(spu, SPURS_TASKSET_REQUEST_WAIT_WKL_FLAG, nullptr, nullptr) != 1)
		{
			rc = spursTasketSaveTaskContext(spu);
			if (rc == CELL_OK)
			{
				incident = CELL_SPURS_TRACE_TASK_WAIT;
			}
		}
		break;
	default:
		rc = CELL_SPURS_TASK_ERROR_NOSYS;
		break;
	}

	if (incident)
	{
		// Trace - TASK
		CellSpursTracePacket pkt{};
		pkt.header.tag = CELL_SPURS_TRACE_TAG_TASK;
		pkt.data.task.incident = incident;
		pkt.data.task.taskId = ctxt->taskId;
		cellSpursModulePutTrace(&pkt, ctxt->dmaTagId);

		// Clear the GUID of the task
		std::memset(spu._ptr<void>(ctxt->guidAddr), 0, 0x10);

		if (spursTasksetPollStatus(spu))
		{
			spursTasksetExit(spu);
		}
		else
		{
			spursTasksetDispatch(spu);
		}
	}

	return rc;
}

// Initialise the Taskset PM
void spursTasksetInit(spu_thread& spu, u32 pollStatus)
{
	auto ctxt = spu._ptr<SpursTasksetContext>(0x2700);
	auto kernelCtxt = spu._ptr<SpursKernelContext>(0x100);

	kernelCtxt->moduleId[0] = 'T';
	kernelCtxt->moduleId[1] = 'K';

	// Trace - START: Module='TKST'
	CellSpursTracePacket pkt{};
	pkt.header.tag = 0x52; // Its not clear what this tag means exactly but it seems similar to CELL_SPURS_TRACE_TAG_START
	std::memcpy(pkt.data.start._module, "TKST", 4);
	pkt.data.start.level = 2;
	pkt.data.start.ls = 0xA00 >> 2;
	cellSpursModulePutTrace(&pkt, ctxt->dmaTagId);

	spursTasksetProcessPollStatus(spu, pollStatus);
}

// Load an ELF
s32 spursTasksetLoadElf(spu_thread& spu, u32* entryPoint, u32* lowestLoadAddr, u64 elfAddr, bool skipWriteableSegments)
{
	if (elfAddr == 0 || (elfAddr & 0x0F) != 0)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	const spu_exec_object obj(fs::file(vm::base(vm::cast(elfAddr)), u32(0 - elfAddr)));

	if (obj != elf_error::ok)
	{
		return CELL_SPURS_TASK_ERROR_NOEXEC;
	}

	u32 _lowestLoadAddr = CELL_SPURS_TASK_BOTTOM;
	for (const auto& prog : obj.progs)
	{
		if (prog.p_paddr >= CELL_SPURS_TASK_BOTTOM)
		{
			break;
		}

		if (prog.p_type == 1u /* PT_LOAD */)
		{
			if (skipWriteableSegments == false || (prog.p_flags & 2u /*PF_W*/ ) == 0u)
			{
				if (prog.p_vaddr < CELL_SPURS_TASK_TOP || prog.p_vaddr + prog.p_memsz > CELL_SPURS_TASK_BOTTOM)
				{
					return CELL_SPURS_TASK_ERROR_FAULT;
				}

				_lowestLoadAddr > prog.p_vaddr ? _lowestLoadAddr = prog.p_vaddr : _lowestLoadAddr;
			}
		}
	}

	for (const auto& prog : obj.progs)
	{
		if (prog.p_paddr >= CELL_SPURS_TASK_BOTTOM) // ???
		{
			break;
		}

		if (prog.p_type == 1u)
		{
			if (skipWriteableSegments == false || (prog.p_flags & 2u) == 0u)
			{
				std::memcpy(spu._ptr<void>(prog.p_vaddr), prog.bin.data(), prog.p_filesz);
			}
		}
	}

	*entryPoint = obj.header.e_entry;
	if (lowestLoadAddr) *lowestLoadAddr = _lowestLoadAddr;

	return CELL_OK;
}

//----------------------------------------------------------------------------
// SPURS taskset policy module functions
//----------------------------------------------------------------------------
bool spursJobChainEntry(spu_thread& /*spu*/)
{
	//const auto ctxt = spu._ptr<SpursJobChainContext>(0x4a00);
	//auto kernelCtxt = spu._ptr<SpursKernelContext>(spu.gpr[3]._u32[3]);

	//auto arg = spu.gpr[4]._u64[1];
	//auto pollStatus = spu.gpr[5]._u32[3];

	// TODO
	return false;
}

void spursJobchainPopUrgentCommand(spu_thread& spu)
{
	const auto ctxt = spu._ptr<SpursJobChainContext>(0x4a00);
	const auto jc = vm::unsafe_ptr_cast<CellSpursJobChain_x00>(+ctxt->jobChain);

	const bool alterQueue = ctxt->unkFlag0;
	vm::reservation_op(spu, jc, [&](CellSpursJobChain_x00& op)
	{
		const auto ls = reinterpret_cast<CellSpursJobChain_x00*>(ctxt->tempAreaJobChain);

		struct alignas(16) { v128 first, second; } data;
		std::memcpy(&data, &op.urgentCmds, sizeof(op.urgentCmds));

		if (!alterQueue)
		{
			// Read the queue, do not modify it
		}
		else
		{
			// Move FIFO queue contents one command up
			data.first._u64[0] = data.first._u64[1];
			data.first._u64[1] = data.second._u64[0];
			data.second._u64[0] = data.second._u64[1];
			data.second._u64[1] = 0;
		}

		// Writeback
		std::memcpy(&ls->urgentCmds, &data, sizeof(op.urgentCmds));

		std::memcpy(&ls->isHalted, &op.unk0[0], 1); // Maybe intended to set it to false
		ls->unk5 = 0;
		ls->sizeJobDescriptor = op.maxGrabbedJob;
		std::memcpy(&op, ls, 128);
	});
}
