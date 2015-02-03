#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/lv2/sys_lwmutex.h"
#include "Emu/SysCalls/lv2/sys_lwcond.h"
#include "Emu/SysCalls/lv2/sys_spu.h"
#include "Emu/SysCalls/Modules/cellSpurs.h"
#include "Loader/ELF32.h"
#include "Emu/FS/vfsStreamMemory.h"

//
// SPURS utility functions
//
void cellSpursModulePutTrace(CellSpursTracePacket * packet, u32 dmaTagId);
u32 cellSpursModulePollStatus(SPUThread & spu, u32 * status);
void cellSpursModuleExit(SPUThread & spu);

bool spursDma(SPUThread & spu, u32 cmd, u64 ea, u32 lsa, u32 size, u32 tag);
u32 spursDmaGetCompletionStatus(SPUThread & spu, u32 tagMask);
u32 spursDmaWaitForCompletion(SPUThread & spu, u32 tagMask, bool waitForAll = true);
void spursHalt(SPUThread & spu);

//
// SPURS Kernel functions
//
bool spursKernel1SelectWorkload(SPUThread & spu);
bool spursKernel2SelectWorkload(SPUThread & spu);
void spursKernelDispatchWorkload(SPUThread & spu, u64 widAndPollStatus);
bool spursKernelWorkloadExit(SPUThread & spu);
bool spursKernelEntry(SPUThread & spu);

//
// SPURS System Service functions
//
bool spursSysServiceEntry(SPUThread & spu);
// TODO: Exit
void spursSysServiceIdleHandler(SPUThread & spu, SpursKernelContext * ctxt);
void spursSysServiceMain(SPUThread & spu, u32 pollStatus);
void spursSysServiceProcessRequests(SPUThread & spu, SpursKernelContext * ctxt);
void spursSysServiceActivateWorkload(SPUThread & spu, SpursKernelContext * ctxt);
// TODO: Deactivate workload
void spursSysServiceUpdateShutdownCompletionEvents(SPUThread & spu, SpursKernelContext * ctxt, u32 wklShutdownBitSet);
void spursSysServiceTraceSaveCount(SPUThread & spu, SpursKernelContext * ctxt);
void spursSysServiceTraceUpdate(SPUThread & spu, SpursKernelContext * ctxt, u32 arg2, u32 arg3, u32 arg4);
// TODO: Deactivate trace
// TODO: System workload entry
void spursSysServiceCleanupAfterSystemWorkload(SPUThread & spu, SpursKernelContext * ctxt);

//
// SPURS Taskset Policy Module functions
//
bool spursTasksetEntry(SPUThread & spu);
bool spursTasksetSyscallEntry(SPUThread & spu);
void spursTasksetResumeTask(SPUThread & spu);
void spursTasksetStartTask(SPUThread & spu, CellSpursTaskArgument & taskArgs);
s32 spursTasksetProcessRequest(SPUThread & spu, s32 request, u32 * taskId, u32 * isWaiting);
void spursTasksetProcessPollStatus(SPUThread & spu, u32 pollStatus);
bool spursTasksetPollStatus(SPUThread & spu);
void spursTasksetExit(SPUThread & spu);
void spursTasksetOnTaskExit(SPUThread & spu, u64 addr, u32 taskId, s32 exitCode, u64 args);
s32 spursTasketSaveTaskContext(SPUThread & spu);
void spursTasksetDispatch(SPUThread & spu);
s32 spursTasksetProcessSyscall(SPUThread & spu, u32 syscallNum, u32 args);
void spursTasksetInit(SPUThread & spu, u32 pollStatus);
s32 spursTasksetLoadElf(SPUThread & spu, u32 * entryPoint, u32 * lowestLoadAddr, u64 elfAddr, bool skipWriteableSegments);

extern Module *cellSpurs;

//////////////////////////////////////////////////////////////////////////////
// SPURS utility functions
//////////////////////////////////////////////////////////////////////////////

/// Output trace information
void cellSpursModulePutTrace(CellSpursTracePacket * packet, u32 dmaTagId) {
    // TODO: Implement this
}

/// Check for execution right requests
u32 cellSpursModulePollStatus(SPUThread & spu, u32 * status) {
    auto ctxt = vm::get_ptr<SpursKernelContext>(spu.ls_offset + 0x100);

    spu.GPR[3]._u32[3] = 1;
    if (ctxt->spurs->m.flags1 & SF1_32_WORKLOADS) {
        spursKernel2SelectWorkload(spu);
    } else {
        spursKernel1SelectWorkload(spu);
    }

    auto result = spu.GPR[3]._u64[1];
    if (status) {
        *status = (u32)result;
    }

    u32 wklId = result >> 32;
    return wklId == ctxt->wklCurrentId ? 0 : 1;
}

/// Exit current workload
void cellSpursModuleExit(SPUThread & spu) {
    auto ctxt = vm::get_ptr<SpursKernelContext>(spu.ls_offset + 0x100);
    spu.SetBranch(ctxt->exitToKernelAddr);
}

/// Execute a DMA operation
bool spursDma(SPUThread & spu, u32 cmd, u64 ea, u32 lsa, u32 size, u32 tag) {
    spu.WriteChannel(MFC_LSA, u128::from32r(lsa));
    spu.WriteChannel(MFC_EAH, u128::from32r((u32)(ea >> 32)));
    spu.WriteChannel(MFC_EAL, u128::from32r((u32)ea));
    spu.WriteChannel(MFC_Size, u128::from32r(size));
    spu.WriteChannel(MFC_TagID, u128::from32r(tag));
    spu.WriteChannel(MFC_Cmd, u128::from32r(cmd));

    if (cmd == MFC_GETLLAR_CMD || cmd == MFC_PUTLLC_CMD || cmd == MFC_PUTLLUC_CMD) {
        u128 rv;

        spu.ReadChannel(rv, MFC_RdAtomicStat);
        auto success = rv._u32[3] ? true : false;
        success      = cmd == MFC_PUTLLC_CMD ? !success : success;
        return success;
    }

    return true;
}

/// Get the status of DMA operations
u32 spursDmaGetCompletionStatus(SPUThread & spu, u32 tagMask) {
    u128 rv;

    spu.WriteChannel(MFC_WrTagMask, u128::from32r(tagMask));
    spu.WriteChannel(MFC_WrTagUpdate, u128::from32r(MFC_TAG_UPDATE_IMMEDIATE));
    spu.ReadChannel(rv, MFC_RdTagStat);
    return rv._u32[3];
}

/// Wait for DMA operations to complete
u32 spursDmaWaitForCompletion(SPUThread & spu, u32 tagMask, bool waitForAll) {
    u128 rv;

    spu.WriteChannel(MFC_WrTagMask, u128::from32r(tagMask));
    spu.WriteChannel(MFC_WrTagUpdate, u128::from32r(waitForAll ? MFC_TAG_UPDATE_ALL : MFC_TAG_UPDATE_ANY));
    spu.ReadChannel(rv, MFC_RdTagStat);
    return rv._u32[3];
}

/// Halt the SPU
void spursHalt(SPUThread & spu) {
    spu.SPU.Status.SetValue(SPU_STATUS_STOPPED_BY_HALT);
    spu.Stop();
}

//////////////////////////////////////////////////////////////////////////////
// SPURS kernel functions
//////////////////////////////////////////////////////////////////////////////

/// Select a workload to run
bool spursKernel1SelectWorkload(SPUThread & spu) {
    auto ctxt = vm::get_ptr<SpursKernelContext>(spu.ls_offset + 0x100);

    // The first and only argument to this function is a boolean that is set to false if the function
    // is called by the SPURS kernel and set to true if called by cellSpursModulePollStatus.
    // If the first argument is true then the shared data is not updated with the result.
    const auto isPoll = spu.GPR[3]._u32[3];

    u32 wklSelectedId;
    u32 pollStatus;

    do {
        // DMA and lock the first 0x80 bytes of spurs
        spursDma(spu, MFC_GETLLAR_CMD, ctxt->spurs.addr(), 0x100/*LSA*/, 0x80/*size*/, 0/*tag*/);
        auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x100);

        // Calculate the contention (number of SPUs used) for each workload
        u8 contention[CELL_SPURS_MAX_WORKLOAD];
        u8 pendingContention[CELL_SPURS_MAX_WORKLOAD];
        for (auto i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
            contention[i] = spurs->m.wklCurrentContention[i] - ctxt->wklLocContention[i];

            // If this is a poll request then the number of SPUs pending to context switch is also added to the contention presumably
            // to prevent unnecessary jumps to the kernel
            if (isPoll) {
                pendingContention[i] = spurs->m.wklPendingContention[i] - ctxt->wklLocPendingContention[i];
                if (i != ctxt->wklCurrentId) {
                    contention[i] += pendingContention[i];
                }
            }
        }

        wklSelectedId = CELL_SPURS_SYS_SERVICE_WORKLOAD_ID;
        pollStatus    = 0;

        // The system service has the highest priority. Select the system service if
        // the system service message bit for this SPU is set.
        if (spurs->m.sysSrvMessage.read_relaxed() & (1 << ctxt->spuNum)) {
            ctxt->spuIdling = 0;
            if (!isPoll || ctxt->wklCurrentId == CELL_SPURS_SYS_SERVICE_WORKLOAD_ID) {
                // Clear the message bit
                spurs->m.sysSrvMessage.write_relaxed(spurs->m.sysSrvMessage.read_relaxed() & ~(1 << ctxt->spuNum));
            }
        } else {
            // Caclulate the scheduling weight for each workload
            u16 maxWeight = 0;
            for (auto i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
                u16 runnable     = ctxt->wklRunnable1 & (0x8000 >> i);
                u16 wklSignal    = spurs->m.wklSignal1.read_relaxed() & (0x8000 >> i);
                u8  wklFlag      = spurs->m.wklFlag.flag.read_relaxed() == 0 ? spurs->m.wklFlagReceiver.read_relaxed() == i ? 1 : 0 : 0;
                u8  readyCount   = spurs->m.wklReadyCount1[i].read_relaxed() > CELL_SPURS_MAX_SPU ? CELL_SPURS_MAX_SPU : spurs->m.wklReadyCount1[i].read_relaxed();
                u8  idleSpuCount = spurs->m.wklIdleSpuCountOrReadyCount2[i].read_relaxed() > CELL_SPURS_MAX_SPU ? CELL_SPURS_MAX_SPU : spurs->m.wklIdleSpuCountOrReadyCount2[i].read_relaxed();
                u8  requestCount = readyCount + idleSpuCount;

                // For a workload to be considered for scheduling:
                // 1. Its priority must not be 0
                // 2. The number of SPUs used by it must be less than the max contention for that workload
                // 3. The workload should be in runnable state
                // 4. The number of SPUs allocated to it must be less than the number of SPUs requested (i.e. readyCount)
                //    OR the workload must be signalled
                //    OR the workload flag is 0 and the workload is configured as the wokload flag receiver
                if (runnable && ctxt->priority[i] != 0 && spurs->m.wklMaxContention[i].read_relaxed() > contention[i]) {
                    if (wklFlag || wklSignal || (readyCount != 0 && requestCount > contention[i])) {
                        // The scheduling weight of the workload is formed from the following parameters in decreasing order of priority:
                        // 1. Wokload signal set or workload flag or ready count > contention
                        // 2. Priority of the workload on the SPU
                        // 3. Is the workload the last selected workload
                        // 4. Minimum contention of the workload
                        // 5. Number of SPUs that are being used by the workload (lesser the number, more the weight)
                        // 6. Is the workload executable same as the currently loaded executable
                        // 7. The workload id (lesser the number, more the weight)
                        u16 weight  = (wklFlag || wklSignal || (readyCount > contention[i])) ? 0x8000 : 0;
                        weight     |= (u16)(ctxt->priority[i] & 0x7F) << 16;
                        weight     |= i == ctxt->wklCurrentId ? 0x80 : 0x00;
                        weight     |= (contention[i] > 0 && spurs->m.wklMinContention[i] > contention[i]) ? 0x40 : 0x00;
                        weight     |= ((CELL_SPURS_MAX_SPU - contention[i]) & 0x0F) << 2;
                        weight     |= ctxt->wklUniqueId[i] == ctxt->wklCurrentId ? 0x02 : 0x00;
                        weight     |= 0x01;

                        // In case of a tie the lower numbered workload is chosen
                        if (weight > maxWeight) {
                            wklSelectedId  = i;
                            maxWeight      = weight;
                            pollStatus     = readyCount > contention[i] ? CELL_SPURS_MODULE_POLL_STATUS_READYCOUNT : 0;
                            pollStatus    |= wklSignal ? CELL_SPURS_MODULE_POLL_STATUS_SIGNAL : 0;
                            pollStatus    |= wklFlag ? CELL_SPURS_MODULE_POLL_STATUS_FLAG : 0;
                        }
                    }
                }
            }

            // Not sure what this does. Possibly mark the SPU as idle/in use.
            ctxt->spuIdling = wklSelectedId == CELL_SPURS_SYS_SERVICE_WORKLOAD_ID ? 1 : 0;

            if (!isPoll || wklSelectedId == ctxt->wklCurrentId) {
                // Clear workload signal for the selected workload
                spurs->m.wklSignal1.write_relaxed(be_t<u16>::make(spurs->m.wklSignal1.read_relaxed() & ~(0x8000 >> wklSelectedId)));
                spurs->m.wklSignal2.write_relaxed(be_t<u16>::make(spurs->m.wklSignal1.read_relaxed() & ~(0x80000000u >> wklSelectedId)));

                // If the selected workload is the wklFlag workload then pull the wklFlag to all 1s
                if (wklSelectedId == spurs->m.wklFlagReceiver.read_relaxed()) {
                    spurs->m.wklFlag.flag.write_relaxed(be_t<u32>::make(0xFFFFFFFF));
                }
            }
        }

        if (!isPoll) {
            // Called by kernel
            // Increment the contention for the selected workload
            if (wklSelectedId != CELL_SPURS_SYS_SERVICE_WORKLOAD_ID) {
                contention[wklSelectedId]++;
            }

            for (auto i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
                spurs->m.wklCurrentContention[i] = contention[i];
                spurs->m.wklPendingContention[i] = spurs->m.wklPendingContention[i] - ctxt->wklLocPendingContention[i];
                ctxt->wklLocContention[i]        = 0;
                ctxt->wklLocPendingContention[i] = 0;
            }

            if (wklSelectedId != CELL_SPURS_SYS_SERVICE_WORKLOAD_ID) {
                ctxt->wklLocContention[wklSelectedId] = 1;
            }

            ctxt->wklCurrentId = wklSelectedId;
        } else if (wklSelectedId != ctxt->wklCurrentId) {
            // Not called by kernel but a context switch is required
            // Increment the pending contention for the selected workload
            if (wklSelectedId != CELL_SPURS_SYS_SERVICE_WORKLOAD_ID) {
                pendingContention[wklSelectedId]++;
            }

            for (auto i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
                spurs->m.wklPendingContention[i] = pendingContention[i];
                ctxt->wklLocPendingContention[i] = 0;
            }

            if (wklSelectedId != CELL_SPURS_SYS_SERVICE_WORKLOAD_ID) {
                ctxt->wklLocPendingContention[wklSelectedId] = 1;
            }
        } else {
            // Not called by kernel and no context switch is required
            for (auto i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
                spurs->m.wklPendingContention[i] = spurs->m.wklPendingContention[i] - ctxt->wklLocPendingContention[i];
                ctxt->wklLocPendingContention[i] = 0;
            }
        }
    } while (spursDma(spu, MFC_PUTLLC_CMD, ctxt->spurs.addr(), 0x100/*LSA*/, 0x80/*size*/, 0/*tag*/) == false);

    u64 result          = (u64)wklSelectedId << 32;
    result             |= pollStatus;
    spu.GPR[3]._u64[1]  = result;
    return true;
}

/// Select a workload to run
bool spursKernel2SelectWorkload(SPUThread & spu) {
    auto ctxt = vm::get_ptr<SpursKernelContext>(spu.ls_offset + 0x100);

    // The first and only argument to this function is a boolean that is set to false if the function
    // is called by the SPURS kernel and set to true if called by cellSpursModulePollStatus.
    // If the first argument is true then the shared data is not updated with the result.
    const auto isPoll = spu.GPR[3]._u32[3];

    u32 wklSelectedId;
    u32 pollStatus;

    do {
        // DMA and lock the first 0x80 bytes of spurs
        spursDma(spu, MFC_GETLLAR_CMD, ctxt->spurs.addr(), 0x100/*LSA*/, 0x80/*size*/, 0/*tag*/);
        auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x100);

        // Calculate the contention (number of SPUs used) for each workload
        u8 contention[CELL_SPURS_MAX_WORKLOAD2];
        u8 pendingContention[CELL_SPURS_MAX_WORKLOAD2];
        for (auto i = 0; i < CELL_SPURS_MAX_WORKLOAD2; i++) {
            contention[i] = spurs->m.wklCurrentContention[i & 0x0F] - ctxt->wklLocContention[i & 0x0F];
            contention[i] = i < CELL_SPURS_MAX_WORKLOAD ? contention[i] & 0x0F : contention[i] >> 4;

            // If this is a poll request then the number of SPUs pending to context switch is also added to the contention presumably
            // to prevent unnecessary jumps to the kernel
            if (isPoll) {
                pendingContention[i] = spurs->m.wklPendingContention[i & 0x0F] - ctxt->wklLocPendingContention[i & 0x0F];
                pendingContention[i] = i < CELL_SPURS_MAX_WORKLOAD ? pendingContention[i] & 0x0F : pendingContention[i] >> 4;
                if (i != ctxt->wklCurrentId) {
                    contention[i] += pendingContention[i];
                }
            }
        }

        wklSelectedId = CELL_SPURS_SYS_SERVICE_WORKLOAD_ID;
        pollStatus    = 0;

        // The system service has the highest priority. Select the system service if
        // the system service message bit for this SPU is set.
        if (spurs->m.sysSrvMessage.read_relaxed() & (1 << ctxt->spuNum)) {
            // Not sure what this does. Possibly Mark the SPU as in use.
            ctxt->spuIdling = 0;
            if (!isPoll || ctxt->wklCurrentId == CELL_SPURS_SYS_SERVICE_WORKLOAD_ID) {
                // Clear the message bit
                spurs->m.sysSrvMessage.write_relaxed(spurs->m.sysSrvMessage.read_relaxed() & ~(1 << ctxt->spuNum));
            }
        } else {
            // Caclulate the scheduling weight for each workload
            u8 maxWeight = 0;
            for (auto i = 0; i < CELL_SPURS_MAX_WORKLOAD2; i++) {
                auto j           = i & 0x0F;
                u16 runnable      = i < CELL_SPURS_MAX_WORKLOAD ? ctxt->wklRunnable1 & (0x8000 >> j) : ctxt->wklRunnable2 & (0x8000 >> j);
                u8  priority      = i < CELL_SPURS_MAX_WORKLOAD ? ctxt->priority[j] & 0x0F : ctxt->priority[j] >> 4;
                u8  maxContention = i < CELL_SPURS_MAX_WORKLOAD ? spurs->m.wklMaxContention[j].read_relaxed() & 0x0F : spurs->m.wklMaxContention[j].read_relaxed() >> 4;
                u16 wklSignal     = i < CELL_SPURS_MAX_WORKLOAD ? spurs->m.wklSignal1.read_relaxed() & (0x8000 >> j) : spurs->m.wklSignal2.read_relaxed() & (0x8000 >> j);
                u8  wklFlag       = spurs->m.wklFlag.flag.read_relaxed() == 0 ? spurs->m.wklFlagReceiver.read_relaxed() == i ? 1 : 0 : 0;
                u8  readyCount    = i < CELL_SPURS_MAX_WORKLOAD ? spurs->m.wklReadyCount1[j].read_relaxed() : spurs->m.wklIdleSpuCountOrReadyCount2[j].read_relaxed();

                // For a workload to be considered for scheduling:
                // 1. Its priority must be greater than 0
                // 2. The number of SPUs used by it must be less than the max contention for that workload
                // 3. The workload should be in runnable state
                // 4. The number of SPUs allocated to it must be less than the number of SPUs requested (i.e. readyCount)
                //    OR the workload must be signalled
                //    OR the workload flag is 0 and the workload is configured as the wokload receiver
                if (runnable && priority > 0 && maxContention > contention[i]) {
                    if (wklFlag || wklSignal || readyCount > contention[i]) {
                        // The scheduling weight of the workload is equal to the priority of the workload for the SPU.
                        // The current workload is given a sligtly higher weight presumably to reduce the number of context switches.
                        // In case of a tie the lower numbered workload is chosen.
                        u8 weight = priority << 4;
                        if (ctxt->wklCurrentId == i) {
                            weight |= 0x04;
                        }

                        if (weight > maxWeight) {
                            wklSelectedId  = i;
                            maxWeight      = weight;
                            pollStatus     = readyCount > contention[i] ? CELL_SPURS_MODULE_POLL_STATUS_READYCOUNT : 0;
                            pollStatus    |= wklSignal ? CELL_SPURS_MODULE_POLL_STATUS_SIGNAL : 0;
                            pollStatus    |= wklFlag ? CELL_SPURS_MODULE_POLL_STATUS_FLAG : 0;
                        }
                    }
                }
            }

            // Not sure what this does. Possibly mark the SPU as idle/in use.
            ctxt->spuIdling = wklSelectedId == CELL_SPURS_SYS_SERVICE_WORKLOAD_ID ? 1 : 0;

            if (!isPoll || wklSelectedId == ctxt->wklCurrentId) {
                // Clear workload signal for the selected workload
                spurs->m.wklSignal1.write_relaxed(be_t<u16>::make(spurs->m.wklSignal1.read_relaxed() & ~(0x8000 >> wklSelectedId)));
                spurs->m.wklSignal2.write_relaxed(be_t<u16>::make(spurs->m.wklSignal1.read_relaxed() & ~(0x80000000u >> wklSelectedId)));

                // If the selected workload is the wklFlag workload then pull the wklFlag to all 1s
                if (wklSelectedId == spurs->m.wklFlagReceiver.read_relaxed()) {
                    spurs->m.wklFlag.flag.write_relaxed(be_t<u32>::make(0xFFFFFFFF));
                }
            }
        }

        if (!isPoll) {
            // Called by kernel
            // Increment the contention for the selected workload
            if (wklSelectedId != CELL_SPURS_SYS_SERVICE_WORKLOAD_ID) {
                contention[wklSelectedId]++;
            }

            for (auto i = 0; i < (CELL_SPURS_MAX_WORKLOAD2 >> 1); i++) {
                spurs->m.wklCurrentContention[i] = contention[i] | (contention[i + 0x10] << 4);
                spurs->m.wklPendingContention[i] = spurs->m.wklPendingContention[i] - ctxt->wklLocPendingContention[i];
                ctxt->wklLocContention[i]        = 0;
                ctxt->wklLocPendingContention[i] = 0;
            }

            ctxt->wklLocContention[wklSelectedId & 0x0F] = wklSelectedId < CELL_SPURS_MAX_WORKLOAD ? 0x01 : wklSelectedId < CELL_SPURS_MAX_WORKLOAD2 ? 0x10 : 0;
            ctxt->wklCurrentId = wklSelectedId;
        } else if (wklSelectedId != ctxt->wklCurrentId) {
            // Not called by kernel but a context switch is required
            // Increment the pending contention for the selected workload
            if (wklSelectedId != CELL_SPURS_SYS_SERVICE_WORKLOAD_ID) {
                pendingContention[wklSelectedId]++;
            }

            for (auto i = 0; i < (CELL_SPURS_MAX_WORKLOAD2 >> 1); i++) {
                spurs->m.wklPendingContention[i] = pendingContention[i] | (pendingContention[i + 0x10] << 4);
                ctxt->wklLocPendingContention[i] = 0;
            }

            ctxt->wklLocPendingContention[wklSelectedId & 0x0F] = wklSelectedId < CELL_SPURS_MAX_WORKLOAD ? 0x01 : wklSelectedId < CELL_SPURS_MAX_WORKLOAD2 ? 0x10 : 0;
        } else {
            // Not called by kernel and no context switch is required
            for (auto i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
                spurs->m.wklPendingContention[i] = spurs->m.wklPendingContention[i] - ctxt->wklLocPendingContention[i];
                ctxt->wklLocPendingContention[i] = 0;
            }
        }
    } while (spursDma(spu, MFC_PUTLLC_CMD, ctxt->spurs.addr(), 0x100/*LSA*/, 0x80/*size*/, 0/*tag*/) == false);

    u64 result          = (u64)wklSelectedId << 32;
    result             |= pollStatus;
    spu.GPR[3]._u64[1]  = result;
    return true;
}

/// SPURS kernel dispatch workload
void spursKernelDispatchWorkload(SPUThread & spu, u64 widAndPollStatus) {
    auto ctxt      = vm::get_ptr<SpursKernelContext>(spu.ls_offset + 0x100);
    auto isKernel2 = ctxt->spurs->m.flags1 & SF1_32_WORKLOADS ? true : false;

    auto pollStatus = (u32)widAndPollStatus;
    auto wid        = (u32)(widAndPollStatus >> 32);

    // DMA in the workload info for the selected workload
    auto wklInfoOffset =  wid < CELL_SPURS_MAX_WORKLOAD ? offsetof(CellSpurs, m.wklInfo1[wid]) :
                                                          wid < CELL_SPURS_MAX_WORKLOAD2 && isKernel2 ? offsetof(CellSpurs, m.wklInfo2[wid & 0xf]) :
                                                                                                        offsetof(CellSpurs, m.wklInfoSysSrv);
    spursDma(spu, MFC_GET_CMD, ctxt->spurs.addr() + wklInfoOffset, 0x3FFE0/*LSA*/, 0x20/*size*/, CELL_SPURS_KERNEL_DMA_TAG_ID);
    spursDmaWaitForCompletion(spu, 0x80000000);

    // Load the workload to LS
    auto wklInfo = vm::get_ptr<CellSpurs::WorkloadInfo>(spu.ls_offset + 0x3FFE0);
    if (ctxt->wklCurrentAddr != wklInfo->addr) {
        switch (wklInfo->addr.addr()) {
        case SPURS_IMG_ADDR_SYS_SRV_WORKLOAD:
            spu.RegisterHleFunction(0xA00, spursSysServiceEntry);
            break;
        case SPURS_IMG_ADDR_TASKSET_PM:
            spu.RegisterHleFunction(0xA00, spursTasksetEntry);
            break;
        default:
            spursDma(spu, MFC_GET_CMD, wklInfo-> addr.addr(), 0xA00/*LSA*/, wklInfo->size, CELL_SPURS_KERNEL_DMA_TAG_ID);
            spursDmaWaitForCompletion(spu, 0x80000000);
            break;
        }

        ctxt->wklCurrentAddr     = wklInfo->addr;
        ctxt->wklCurrentUniqueId = wklInfo->uniqueId.read_relaxed();
    }

    if (!isKernel2) {
        ctxt->moduleId[0] = 0;
        ctxt->moduleId[1] = 0;
    }

    // Run workload
    spu.GPR[0]._u32[3] = ctxt->exitToKernelAddr;
    spu.GPR[1]._u32[3] = 0x3FFB0;
    spu.GPR[3]._u32[3] = 0x100;
    spu.GPR[4]._u64[1] = wklInfo->arg;
    spu.GPR[5]._u32[3] = pollStatus;
    spu.SetBranch(0xA00);
}

/// SPURS kernel workload exit
bool spursKernelWorkloadExit(SPUThread & spu) {
    auto ctxt      = vm::get_ptr<SpursKernelContext>(spu.ls_offset + 0x100);
    auto isKernel2 = ctxt->spurs->m.flags1 & SF1_32_WORKLOADS ? true : false;

    // Select next workload to run
    spu.GPR[3].clear();
    if (isKernel2) {
        spursKernel2SelectWorkload(spu);
    } else {
        spursKernel1SelectWorkload(spu);
    }

    spursKernelDispatchWorkload(spu, spu.GPR[3]._u64[1]);
    return false;
}

/// SPURS kernel entry point
bool spursKernelEntry(SPUThread & spu) {
    auto ctxt = vm::get_ptr<SpursKernelContext>(spu.ls_offset + 0x100);
    memset(ctxt, 0, sizeof(SpursKernelContext));

    // Save arguments
    ctxt->spuNum = spu.GPR[3]._u32[3];
    ctxt->spurs.set(spu.GPR[4]._u64[1]);

    auto isKernel2 = ctxt->spurs->m.flags1 & SF1_32_WORKLOADS ? true : false;

    // Initialise the SPURS context to its initial values
    ctxt->dmaTagId           = CELL_SPURS_KERNEL_DMA_TAG_ID;
    ctxt->wklCurrentUniqueId = 0x20;
    ctxt->wklCurrentId       = CELL_SPURS_SYS_SERVICE_WORKLOAD_ID;
    ctxt->exitToKernelAddr   = isKernel2 ? CELL_SPURS_KERNEL2_EXIT_ADDR : CELL_SPURS_KERNEL1_EXIT_ADDR;
    ctxt->selectWorkloadAddr = isKernel2 ? CELL_SPURS_KERNEL2_SELECT_WORKLOAD_ADDR : CELL_SPURS_KERNEL1_SELECT_WORKLOAD_ADDR;
    if (!isKernel2) {
        ctxt->x1F0    = 0xF0020000;
        ctxt->x200    = 0x20000;
        ctxt->guid[0] = 0x423A3A02;
        ctxt->guid[1] = 0x43F43A82;
        ctxt->guid[2] = 0x43F26502;
        ctxt->guid[3] = 0x420EB382;
    } else {
        ctxt->guid[0] = 0x43A08402;
        ctxt->guid[1] = 0x43FB0A82;
        ctxt->guid[2] = 0x435E9302;
        ctxt->guid[3] = 0x43A3C982;
    }

    // Register SPURS kernel HLE functions
    spu.UnregisterHleFunctions(0, 0x40000/*LS_BOTTOM*/);
    spu.RegisterHleFunction(isKernel2 ? CELL_SPURS_KERNEL2_ENTRY_ADDR : CELL_SPURS_KERNEL1_ENTRY_ADDR, spursKernelEntry);
    spu.RegisterHleFunction(ctxt->exitToKernelAddr, spursKernelWorkloadExit);
    spu.RegisterHleFunction(ctxt->selectWorkloadAddr, isKernel2 ? spursKernel2SelectWorkload : spursKernel1SelectWorkload);

    // Start the system service
    spursKernelDispatchWorkload(spu, ((u64)CELL_SPURS_SYS_SERVICE_WORKLOAD_ID) << 32);
    return false;
}

//////////////////////////////////////////////////////////////////////////////
// SPURS system workload functions
//////////////////////////////////////////////////////////////////////////////

/// Entry point of the system service
bool spursSysServiceEntry(SPUThread & spu) {
    auto ctxt       = vm::get_ptr<SpursKernelContext>(spu.ls_offset + spu.GPR[3]._u32[3]);
    auto arg        = spu.GPR[4]._u64[1];
    auto pollStatus = spu.GPR[5]._u32[3];

    if (ctxt->wklCurrentId == CELL_SPURS_SYS_SERVICE_WORKLOAD_ID) {
        spursSysServiceMain(spu, pollStatus);
    } else {
        // TODO: If we reach here it means the current workload was preempted to start the
        // system workload. Need to implement this.
    }
    
    cellSpursModuleExit(spu);
    return false;
}

/// Wait for an external event or exit the SPURS thread group if no workloads can be scheduled
void spursSysServiceIdleHandler(SPUThread & spu, SpursKernelContext * ctxt) {
    // Monitor only lock line reservation lost events
    spu.WriteChannel(SPU_WrEventMask, u128::from32r(SPU_EVENT_LR));

    bool shouldExit;
    while (true) {
        spursDma(spu, MFC_GETLLAR_CMD, ctxt->spurs.addr(), 0x100/*LSA*/, 0x80/*size*/, 0/*tag*/);
        auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x100);

        // Find the number of SPUs that are idling in this SPURS instance
        u32 nIdlingSpus = 0;
        for (u32 i = 0; i < 8; i++) {
            if (spurs->m.spuIdling & (1 << i)) {
                nIdlingSpus++;
            }
        }

        bool allSpusIdle  = nIdlingSpus == spurs->m.nSpus ? true: false;
        bool exitIfNoWork = spurs->m.flags1 & SF1_EXIT_IF_NO_WORK ? true : false;
        shouldExit        = allSpusIdle && exitIfNoWork;

        // Check if any workloads can be scheduled
        bool foundReadyWorkload = false;
        if (spurs->m.sysSrvMessage.read_relaxed() & (1 << ctxt->spuNum)) {
            foundReadyWorkload = true;
        } else {
            if (spurs->m.flags1 & SF1_32_WORKLOADS) {
                for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD2; i++) {
                    u32 j            = i & 0x0F;
                    u16 runnable     = i < CELL_SPURS_MAX_WORKLOAD ? ctxt->wklRunnable1 & (0x8000 >> j) : ctxt->wklRunnable2 & (0x8000 >> j);
                    u8 priority      = i < CELL_SPURS_MAX_WORKLOAD ? ctxt->priority[j] & 0x0F : ctxt->priority[j] >> 4;
                    u8 maxContention = i < CELL_SPURS_MAX_WORKLOAD ? spurs->m.wklMaxContention[j].read_relaxed() & 0x0F : spurs->m.wklMaxContention[j].read_relaxed() >> 4;
                    u8 contention    = i < CELL_SPURS_MAX_WORKLOAD ? spurs->m.wklCurrentContention[j] & 0x0F : spurs->m.wklCurrentContention[j] >> 4;
                    u16 wklSignal    = i < CELL_SPURS_MAX_WORKLOAD ? spurs->m.wklSignal1.read_relaxed() & (0x8000 >> j) : spurs->m.wklSignal2.read_relaxed() & (0x8000 >> j);
                    u8 wklFlag       = spurs->m.wklFlag.flag.read_relaxed() == 0 ? spurs->m.wklFlagReceiver.read_relaxed() == i ? 1 : 0 : 0;
                    u8 readyCount    = i < CELL_SPURS_MAX_WORKLOAD ? spurs->m.wklReadyCount1[j].read_relaxed() : spurs->m.wklIdleSpuCountOrReadyCount2[j].read_relaxed();

                    if (runnable && priority > 0 && maxContention > contention) {
                        if (wklFlag || wklSignal || readyCount > contention) {
                            foundReadyWorkload = true;
                            break;
                        }
                    }
                }
            } else {
                for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
                    u16 runnable    = ctxt->wklRunnable1 & (0x8000 >> i);
                    u16 wklSignal   = spurs->m.wklSignal1.read_relaxed() & (0x8000 >> i);
                    u8 wklFlag      = spurs->m.wklFlag.flag.read_relaxed() == 0 ? spurs->m.wklFlagReceiver.read_relaxed() == i ? 1 : 0 : 0;
                    u8 readyCount   = spurs->m.wklReadyCount1[i].read_relaxed() > CELL_SPURS_MAX_SPU ? CELL_SPURS_MAX_SPU : spurs->m.wklReadyCount1[i].read_relaxed();
                    u8 idleSpuCount = spurs->m.wklIdleSpuCountOrReadyCount2[i].read_relaxed() > CELL_SPURS_MAX_SPU ? CELL_SPURS_MAX_SPU : spurs->m.wklIdleSpuCountOrReadyCount2[i].read_relaxed();
                    u8 requestCount = readyCount + idleSpuCount;

                    if (runnable && ctxt->priority[i] != 0 && spurs->m.wklMaxContention[i].read_relaxed() > spurs->m.wklCurrentContention[i]) {
                        if (wklFlag || wklSignal || (readyCount != 0 && requestCount > spurs->m.wklCurrentContention[i])) {
                            foundReadyWorkload = true;
                            break;
                        }
                    }
                }
            }
        }

        bool spuIdling = spurs->m.spuIdling & (1 << ctxt->spuNum) ? true : false;
        if (foundReadyWorkload && shouldExit == false) {
            spurs->m.spuIdling &= ~(1 << ctxt->spuNum);
        } else {
            spurs->m.spuIdling |= 1 << ctxt->spuNum;
        }

        // If all SPUs are idling and the exit_if_no_work flag is set then the SPU thread group must exit. Otherwise wait for external events.
        if (spuIdling && shouldExit == false && foundReadyWorkload == false) {
            // The system service blocks by making a reservation and waiting on the lock line reservation lost event.
            u128 r;
            spu.ReadChannel(r, SPU_RdEventStat);
            spu.WriteChannel(SPU_WrEventAck, u128::from32r(SPU_EVENT_LR));
        }

        auto dmaSuccess = spursDma(spu, MFC_PUTLLC_CMD, ctxt->spurs.addr(), 0x100/*LSA*/, 0x80/*size*/, 0/*tag*/);
        if (dmaSuccess && (shouldExit || foundReadyWorkload)) {
            break;
        }
    }

    if (shouldExit) {
        // TODO: exit spu thread group
    }
}

/// Main function for the system service
void spursSysServiceMain(SPUThread & spu, u32 pollStatus) {
    auto ctxt = vm::get_ptr<SpursKernelContext>(spu.ls_offset + 0x100);

    if (ctxt->spurs.addr() % CellSpurs::align) {
        spursHalt(spu);
        return;
    }

    // Initialise the system service if this is the first time its being started on this SPU
    if (ctxt->sysSrvInitialised == 0) {
        ctxt->sysSrvInitialised = 1;

        spursDma(spu, MFC_GETLLAR_CMD, ctxt->spurs.addr(), 0x100/*LSA*/, 0x80/*size*/, 0/*tag*/);

        do {
            spursDma(spu, MFC_GETLLAR_CMD, ctxt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/);
            auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x2D80 - offsetof(CellSpurs, m.wklState1));

            // Halt if already initialised
            if (spurs->m.sysSrvOnSpu & (1 << ctxt->spuNum)) {
                spursHalt(spu);
                return;
            }

            spurs->m.sysSrvOnSpu |= 1 << ctxt->spuNum;
        } while (spursDma(spu, MFC_PUTLLC_CMD, ctxt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/) == false);

        ctxt->traceBuffer   = 0;
        ctxt->traceMsgCount = -1;
        spursSysServiceTraceUpdate(spu, ctxt, 1, 1, 0);
        spursSysServiceCleanupAfterSystemWorkload(spu, ctxt);

        // Trace - SERVICE: INIT
        CellSpursTracePacket pkt;
        memset(&pkt, 0, sizeof(pkt));
        pkt.header.tag            = CELL_SPURS_TRACE_TAG_SERVICE;
        pkt.data.service.incident = CELL_SPURS_TRACE_SERVICE_INIT;
        cellSpursModulePutTrace(&pkt, ctxt->dmaTagId);
    }

    // Trace - START: Module='SYS '
    CellSpursTracePacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.header.tag = CELL_SPURS_TRACE_TAG_START;
    memcpy(pkt.data.start.module, "SYS ", 4);
    pkt.data.start.level = 1; // Policy module
    pkt.data.start.ls    = 0xA00 >> 2;
    cellSpursModulePutTrace(&pkt, ctxt->dmaTagId);

    while (true) {
        // Process requests for the system service
        spursSysServiceProcessRequests(spu, ctxt);

poll:
        if (cellSpursModulePollStatus(spu, nullptr)) {
            // Trace - SERVICE: EXIT
            CellSpursTracePacket pkt;
            memset(&pkt, 0, sizeof(pkt));
            pkt.header.tag            = CELL_SPURS_TRACE_TAG_SERVICE;
            pkt.data.service.incident = CELL_SPURS_TRACE_SERVICE_EXIT;
            cellSpursModulePutTrace(&pkt, ctxt->dmaTagId);

            // Trace - STOP: GUID
            memset(&pkt, 0, sizeof(pkt));
            pkt.header.tag = CELL_SPURS_TRACE_TAG_STOP;
            pkt.data.stop  = SPURS_GUID_SYS_WKL;
            cellSpursModulePutTrace(&pkt, ctxt->dmaTagId);

            spursDmaWaitForCompletion(spu, 1 << ctxt->dmaTagId);
            break;
        }

        // If we reach here it means that either there are more system service messages to be processed
        // or there are no workloads that can be scheduled.

        // If the SPU is not idling then process the remaining system service messages
        if (ctxt->spuIdling == 0) {
            continue;
        }

        // If we reach here it means that the SPU is idling

        // Trace - SERVICE: WAIT
        CellSpursTracePacket pkt;
        memset(&pkt, 0, sizeof(pkt));
        pkt.header.tag            = CELL_SPURS_TRACE_TAG_SERVICE;
        pkt.data.service.incident = CELL_SPURS_TRACE_SERVICE_WAIT;
        cellSpursModulePutTrace(&pkt, ctxt->dmaTagId);

        spursSysServiceIdleHandler(spu, ctxt);
        goto poll;
    }
}

/// Process any requests
void spursSysServiceProcessRequests(SPUThread & spu, SpursKernelContext * ctxt) {
    bool updateTrace    = false;
    bool updateWorkload = false;
    bool terminate      = false;

    do {
        spursDma(spu, MFC_GETLLAR_CMD, ctxt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/);
        auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x2D80 - offsetof(CellSpurs, m.wklState1));

        // Terminate request
        if (spurs->m.sysSrvMsgTerminate & (1 << ctxt->spuNum)) {
            spurs->m.sysSrvOnSpu &= ~(1 << ctxt->spuNum);
            terminate = true;
        }

        // Update workload message
        if (spurs->m.sysSrvMsgUpdateWorkload.read_relaxed() & (1 << ctxt->spuNum)) {
            spurs->m.sysSrvMsgUpdateWorkload &= ~(1 << ctxt->spuNum);
            updateWorkload = true;
        }

        // Update trace message
        if (spurs->m.sysSrvMsgUpdateTrace & (1 << ctxt->spuNum)) {
            updateTrace = true;
        }
    } while (spursDma(spu, MFC_PUTLLC_CMD, ctxt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/) == false);

    // Process update workload message
    if (updateWorkload) {
        spursSysServiceActivateWorkload(spu, ctxt);
    }

    // Process update trace message
    if (updateTrace) {
        spursSysServiceTraceUpdate(spu, ctxt, 1, 0, 0);
    }

    // Process terminate request
    if (terminate) {
        // TODO: Rest of the terminate processing
    }
}

/// Activate a workload
void spursSysServiceActivateWorkload(SPUThread & spu, SpursKernelContext * ctxt) {
    auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x100);
    spursDma(spu, MFC_GET_CMD, ctxt->spurs.addr() + offsetof(CellSpurs, m.wklInfo1), 0x30000/*LSA*/, 0x200/*size*/, CELL_SPURS_KERNEL_DMA_TAG_ID);
    if (spurs->m.flags1 & SF1_32_WORKLOADS) {
        spursDma(spu, MFC_GET_CMD, ctxt->spurs.addr() + offsetof(CellSpurs, m.wklInfo2), 0x30200/*LSA*/, 0x200/*size*/, CELL_SPURS_KERNEL_DMA_TAG_ID);
    }

    u32 wklShutdownBitSet = 0;
    ctxt->wklRunnable1    = 0;
    ctxt->wklRunnable2    = 0;
    for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
        auto wklInfo1 = vm::get_ptr<CellSpurs::WorkloadInfo>(spu.ls_offset + 0x30000);

        // Copy the priority of the workload for this SPU and its unique id to the LS
        ctxt->priority[i]    = wklInfo1[i].priority[ctxt->spuNum] == 0 ? 0 : 0x10 - wklInfo1[i].priority[ctxt->spuNum];
        ctxt->wklUniqueId[i] = wklInfo1[i].uniqueId.read_relaxed();

        if (spurs->m.flags1 & SF1_32_WORKLOADS) {
            auto wklInfo2 = vm::get_ptr<CellSpurs::WorkloadInfo>(spu.ls_offset + 0x30200);

            // Copy the priority of the workload for this SPU to the LS
            if (wklInfo2[i].priority[ctxt->spuNum]) {
                ctxt->priority[i] |= (0x10 - wklInfo2[i].priority[ctxt->spuNum]) << 4;
            }
        }
    }

    do {
        spursDma(spu, MFC_GETLLAR_CMD, ctxt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/);
        spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x2D80 - offsetof(CellSpurs, m.wklState1));

        for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
            // Update workload status and runnable flag based on the workload state
            auto wklStatus = spurs->m.wklStatus1[i];
            if (spurs->m.wklState1[i].read_relaxed() == SPURS_WKL_STATE_RUNNABLE) {
                spurs->m.wklStatus1[i] |= 1 << ctxt->spuNum;
                ctxt->wklRunnable1     |= 0x8000 >> i;
            } else {
                spurs->m.wklStatus1[i] &= ~(1 << ctxt->spuNum);
            }

            // If the workload is shutting down and if this is the last SPU from which it is being removed then
            // add it to the shutdown bit set
            if (spurs->m.wklState1[i].read_relaxed() == SPURS_WKL_STATE_SHUTTING_DOWN) {
                if (((wklStatus & (1 << ctxt->spuNum)) != 0) && (spurs->m.wklStatus1[i] == 0)) {
                    spurs->m.wklState1[i].write_relaxed(SPURS_WKL_STATE_REMOVABLE);
                    wklShutdownBitSet |= 0x80000000u >> i;
                }
            }

            if (spurs->m.flags1 & SF1_32_WORKLOADS) {
                // Update workload status and runnable flag based on the workload state
                wklStatus = spurs->m.wklStatus2[i];
                if (spurs->m.wklState2[i].read_relaxed() == SPURS_WKL_STATE_RUNNABLE) {
                    spurs->m.wklStatus2[i] |= 1 << ctxt->spuNum;
                    ctxt->wklRunnable2     |= 0x8000 >> i;
                } else {
                    spurs->m.wklStatus2[i] &= ~(1 << ctxt->spuNum);
                }

                // If the workload is shutting down and if this is the last SPU from which it is being removed then
                // add it to the shutdown bit set
                if (spurs->m.wklState2[i].read_relaxed() == SPURS_WKL_STATE_SHUTTING_DOWN) {
                    if (((wklStatus & (1 << ctxt->spuNum)) != 0) && (spurs->m.wklStatus2[i] == 0)) {
                        spurs->m.wklState2[i].write_relaxed(SPURS_WKL_STATE_REMOVABLE);
                        wklShutdownBitSet |= 0x8000 >> i;
                    }
                }
            }
        }
    } while (spursDma(spu, MFC_PUTLLC_CMD, ctxt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/) == false);

    if (wklShutdownBitSet) {
        spursSysServiceUpdateShutdownCompletionEvents(spu, ctxt, wklShutdownBitSet);
    }
}

/// Update shutdown completion events
void spursSysServiceUpdateShutdownCompletionEvents(SPUThread & spu, SpursKernelContext * ctxt, u32 wklShutdownBitSet) {
    // Mark the workloads in wklShutdownBitSet as completed and also generate a bit set of the completed
    // workloads that have a shutdown completion hook registered
    u32 wklNotifyBitSet;
    u8  spuPort;
    do {
        spursDma(spu, MFC_GETLLAR_CMD, ctxt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/);
        auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x2D80 - offsetof(CellSpurs, m.wklState1));

        wklNotifyBitSet = 0;
        spuPort         = spurs->m.spuPort;;
        for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
            if (wklShutdownBitSet & (0x80000000u >> i)) {
                spurs->m.wklEvent1[i] |= 0x01;
                if (spurs->m.wklEvent1[i] & 0x02 || spurs->m.wklEvent1[i] & 0x10) {
                    wklNotifyBitSet |= 0x80000000u >> i;
                }
            }

            if (wklShutdownBitSet & (0x8000 >> i)) {
                spurs->m.wklEvent2[i] |= 0x01;
                if (spurs->m.wklEvent2[i] & 0x02 || spurs->m.wklEvent2[i] & 0x10) {
                    wklNotifyBitSet |= 0x8000 >> i;
                }
            }
        }
    } while (spursDma(spu, MFC_PUTLLC_CMD, ctxt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/) == false);

    if (wklNotifyBitSet) {
        // TODO: sys_spu_thread_send_event(spuPort, 0, wklNotifyMask);
    }
}

/// Update the trace count for this SPU
void spursSysServiceTraceSaveCount(SPUThread & spu, SpursKernelContext * ctxt) {
    if (ctxt->traceBuffer) {
        auto traceInfo = vm::ptr<CellSpursTraceInfo>::make((u32)(ctxt->traceBuffer - (ctxt->spurs->m.traceStartIndex[ctxt->spuNum] << 4)));
        traceInfo->count[ctxt->spuNum] = ctxt->traceMsgCount;
    }
}

/// Update trace control
void spursSysServiceTraceUpdate(SPUThread & spu, SpursKernelContext * ctxt, u32 arg2, u32 arg3, u32 arg4) {
    bool notify;

    u8 sysSrvMsgUpdateTrace;
    do {
        spursDma(spu, MFC_GETLLAR_CMD, ctxt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/);
        auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x2D80 - offsetof(CellSpurs, m.wklState1));

        sysSrvMsgUpdateTrace           = spurs->m.sysSrvMsgUpdateTrace;
        spurs->m.sysSrvMsgUpdateTrace &= ~(1 << ctxt->spuNum);
        spurs->m.xCC                  &= ~(1 << ctxt->spuNum);
        spurs->m.xCC                  |= arg2 << ctxt->spuNum;

        notify = false;
        if (((sysSrvMsgUpdateTrace & (1 << ctxt->spuNum)) != 0) && (spurs->m.sysSrvMsgUpdateTrace == 0) && (spurs->m.xCD != 0)) {
            spurs->m.xCD = 0;
            notify       = true;
        }

        if (arg4 && spurs->m.xCD != 0) {
            spurs->m.xCD = 0;
            notify       = true;
        }
    } while (spursDma(spu, MFC_PUTLLC_CMD, ctxt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/) == false);

    // Get trace parameters from CellSpurs and store them in the LS
    if (((sysSrvMsgUpdateTrace & (1 << ctxt->spuNum)) != 0) || (arg3 != 0)) {
        spursDma(spu, MFC_GETLLAR_CMD, ctxt->spurs.addr() + offsetof(CellSpurs, m.traceBuffer), 0x80/*LSA*/, 0x80/*size*/, 0/*tag*/);
        auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x80 - offsetof(CellSpurs, m.traceBuffer));

        if (ctxt->traceMsgCount != 0xFF || spurs->m.traceBuffer.addr() == 0) {
            spursSysServiceTraceSaveCount(spu, ctxt);
        } else {
            spursDma(spu, MFC_GET_CMD, spurs->m.traceBuffer.addr() & 0xFFFFFFFC, 0x2C00/*LSA*/, 0x80/*size*/, ctxt->dmaTagId);
            auto traceBuffer    = vm::get_ptr<CellSpursTraceInfo>(spu.ls_offset + 0x2C00);
            ctxt->traceMsgCount = traceBuffer->count[ctxt->spuNum];
        }

        ctxt->traceBuffer   = spurs->m.traceBuffer.addr() + (spurs->m.traceStartIndex[ctxt->spuNum] << 4);
        ctxt->traceMaxCount = spurs->m.traceStartIndex[1] - spurs->m.traceStartIndex[0];
        if (ctxt->traceBuffer == 0) {
            ctxt->traceMsgCount = 0;
        }
    }

    if (notify) {
        auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x2D80 - offsetof(CellSpurs, m.wklState1));
        sys_spu_thread_send_event(spu, spurs->m.spuPort, 2, 0);
    }
}

/// Restore state after executing the system workload
void spursSysServiceCleanupAfterSystemWorkload(SPUThread & spu, SpursKernelContext * ctxt) {
    u8 wklId;

    do {
        spursDma(spu, MFC_GETLLAR_CMD, ctxt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/);
        auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x2D80 - offsetof(CellSpurs, m.wklState1));

        if (spurs->m.sysSrvWorkload[ctxt->spuNum] == 0xFF) {
            return;
        }

        wklId = spurs->m.sysSrvWorkload[ctxt->spuNum];
        spurs->m.sysSrvWorkload[ctxt->spuNum] = 0xFF;
    } while (spursDma(spu, MFC_PUTLLC_CMD, ctxt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/) == false);

    spursSysServiceActivateWorkload(spu, ctxt);

    do {
        spursDma(spu, MFC_GETLLAR_CMD, ctxt->spurs.addr(), 0x100/*LSA*/, 0x80/*size*/, 0/*tag*/);
        auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x100);

        if (wklId >= CELL_SPURS_MAX_WORKLOAD) {
            spurs->m.wklCurrentContention[wklId & 0x0F] -= 0x10;
            spurs->m.wklReadyCount1[wklId & 0x0F].write_relaxed(spurs->m.wklReadyCount1[wklId & 0x0F].read_relaxed() - 1);
        } else {
            spurs->m.wklCurrentContention[wklId & 0x0F] -= 0x01;
            spurs->m.wklIdleSpuCountOrReadyCount2[wklId & 0x0F].write_relaxed(spurs->m.wklIdleSpuCountOrReadyCount2[wklId & 0x0F].read_relaxed() - 1);
        }
    } while (spursDma(spu, MFC_PUTLLC_CMD, ctxt->spurs.addr(), 0x100/*LSA*/, 0x80/*size*/, 0/*tag*/) == false);

    // Set the current workload id to the id of the pre-empted workload since cellSpursModulePutTrace
    // uses the current worload id to determine the workload to which the trace belongs
    auto wklIdSaved    = ctxt->wklCurrentId;
    ctxt->wklCurrentId = wklId;

    // Trace - STOP: GUID
    CellSpursTracePacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.header.tag = CELL_SPURS_TRACE_TAG_STOP;
    pkt.data.stop  = SPURS_GUID_SYS_WKL;
    cellSpursModulePutTrace(&pkt, ctxt->dmaTagId);

    ctxt->wklCurrentId = wklIdSaved;
}

//////////////////////////////////////////////////////////////////////////////
// SPURS taskset policy module functions
//////////////////////////////////////////////////////////////////////////////

enum SpursTasksetRequest {
    SPURS_TASKSET_REQUEST_POLL_SIGNAL   = -1,
    SPURS_TASKSET_REQUEST_DESTROY_TASK  = 0,
    SPURS_TASKSET_REQUEST_YIELD_TASK    = 1,
    SPURS_TASKSET_REQUEST_WAIT_SIGNAL   = 2,
    SPURS_TASKSET_REQUEST_POLL          = 3,
    SPURS_TASKSET_REQUEST_WAIT_WKL_FLAG = 4,
    SPURS_TASKSET_REQUEST_SELECT_TASK   = 5,
    SPURS_TASKSET_REQUEST_RECV_WKL_FLAG = 6,
};

/// Taskset PM entry point
bool spursTasksetEntry(SPUThread & spu) {
    auto ctxt       = vm::get_ptr<SpursTasksetContext>(spu.ls_offset + 0x2700);
    auto kernelCtxt = vm::get_ptr<SpursKernelContext>(spu.ls_offset + spu.GPR[3]._u32[3]);

    auto arg        = spu.GPR[4]._u64[1];
    auto pollStatus = spu.GPR[5]._u32[3];

    // Initialise memory and save args
    memset(ctxt, 0, sizeof(*ctxt));
    ctxt->taskset.set(arg);
    memcpy(ctxt->moduleId, "SPURSTASK MODULE", sizeof(ctxt->moduleId));
    ctxt->kernelMgmtAddr = spu.GPR[3]._u32[3];
    ctxt->syscallAddr    = CELL_SPURS_TASKSET_PM_SYSCALL_ADDR;
    ctxt->spuNum         = kernelCtxt->spuNum;
    ctxt->dmaTagId       = kernelCtxt->dmaTagId;
    ctxt->taskId         = 0xFFFFFFFF;

    // Register SPURS takset policy module HLE functions
    spu.UnregisterHleFunctions(CELL_SPURS_TASKSET_PM_ENTRY_ADDR, 0x40000/*LS_BOTTOM*/);
    spu.RegisterHleFunction(CELL_SPURS_TASKSET_PM_ENTRY_ADDR, spursTasksetEntry);
    spu.RegisterHleFunction(ctxt->syscallAddr, spursTasksetSyscallEntry);

    // Initialise the taskset policy module
    spursTasksetInit(spu, pollStatus);

    // Dispatch
    spursTasksetDispatch(spu);
    return false;
}

/// Entry point into the Taskset PM for task syscalls
bool spursTasksetSyscallEntry(SPUThread & spu) {
    auto ctxt = vm::get_ptr<SpursTasksetContext>(spu.ls_offset + 0x2700);

    // Save task context
    ctxt->savedContextLr = spu.GPR[0];
    ctxt->savedContextSp = spu.GPR[1];
    for (auto i = 0; i < 48; i++) {
        ctxt->savedContextR80ToR127[i] = spu.GPR[80 + i];
    }

    // Handle the syscall
    spu.GPR[3]._u32[3] = spursTasksetProcessSyscall(spu, spu.GPR[3]._u32[3], spu.GPR[4]._u32[3]);

    // Resume the previously executing task if the syscall did not cause a context switch
    if (spu.m_is_branch == false) {
        spursTasksetResumeTask(spu);
    }

    return false;
}

/// Resume a task
void spursTasksetResumeTask(SPUThread & spu) {
    auto ctxt = vm::get_ptr<SpursTasksetContext>(spu.ls_offset + 0x2700);

    // Restore task context
    spu.GPR[0] = ctxt->savedContextLr;
    spu.GPR[1] = ctxt->savedContextSp;
    for (auto i = 0; i < 48; i++) {
        spu.GPR[80 + i] = ctxt->savedContextR80ToR127[i];
    }

    spu.SetBranch(spu.GPR[0]._u32[3]);
}

/// Start a task
void spursTasksetStartTask(SPUThread & spu, CellSpursTaskArgument & taskArgs) {
    auto ctxt    = vm::get_ptr<SpursTasksetContext>(spu.ls_offset + 0x2700);
    auto taskset = vm::get_ptr<CellSpursTaskset>(spu.ls_offset + 0x2700);

    spu.GPR[2].clear();
    spu.GPR[3]         = u128::from64(taskArgs.u64[0], taskArgs.u64[1]);
    spu.GPR[4]._u64[1] = taskset->m.args;
    spu.GPR[4]._u64[0] = taskset->m.spurs.addr();
    for (auto i = 5; i < 128; i++) {
        spu.GPR[i].clear();
    }

    spu.SetBranch(ctxt->savedContextLr.value()._u32[3]);
}

/// Process a request and update the state of the taskset
s32 spursTasksetProcessRequest(SPUThread & spu, s32 request, u32 * taskId, u32 * isWaiting) {
    auto kernelCtxt = vm::get_ptr<SpursKernelContext>(spu.ls_offset + 0x100);
    auto ctxt       = vm::get_ptr<SpursTasksetContext>(spu.ls_offset + 0x2700);

    s32 rc = CELL_OK;
    s32 numNewlyReadyTasks;
    do {
        spursDma(spu, MFC_GETLLAR_CMD, ctxt->taskset.addr(), 0x2700/*LSA*/, 0x80/*size*/, 0/*tag*/);
        auto taskset = vm::get_ptr<CellSpursTaskset>(spu.ls_offset + 0x2700);

        // Verify taskset state is valid
        auto _0 = be_t<u128>::make(u128::from32(0));
        if ((taskset->m.waiting & taskset->m.running) != _0 || (taskset->m.ready & taskset->m.pending_ready) != _0 ||
            ((taskset->m.running | taskset->m.ready | taskset->m.pending_ready | taskset->m.signalled | taskset->m.waiting) & be_t<u128>::make(~taskset->m.enabled.value())) != _0) {
            spursHalt(spu);
            return CELL_OK;
        }

        // Find the number of tasks that have become ready since the last iteration
        auto newlyReadyTasks = (taskset->m.signalled | taskset->m.pending_ready).value() & ~taskset->m.ready.value();
        numNewlyReadyTasks   = 0;
        for (auto i = 0; i < 128; i++) {
            if (newlyReadyTasks._bit[i]) {
                numNewlyReadyTasks++;
            }
        }

        u128 readyButNotRunning;
        u8   selectedTaskId;
        auto running   = taskset->m.running.value();
        auto waiting   = taskset->m.waiting.value();
        auto enabled   = taskset->m.enabled.value();
        auto signalled = (taskset->m.signalled & (taskset->m.ready | taskset->m.pending_ready)).value();
        auto ready     = (taskset->m.signalled | taskset->m.ready | taskset->m.pending_ready).value();

        switch (request) {
        case SPURS_TASKSET_REQUEST_POLL_SIGNAL:
            rc = signalled._bit[ctxt->taskId] ? 1 : 0;
            signalled._bit[ctxt->taskId] = false;
            break;
        case SPURS_TASKSET_REQUEST_DESTROY_TASK:
            numNewlyReadyTasks--;
            running._bit[ctxt->taskId]   = false;
            enabled._bit[ctxt->taskId]   = false;
            signalled._bit[ctxt->taskId] = false;
            ready._bit[ctxt->taskId]     = false;
            break;
        case SPURS_TASKSET_REQUEST_YIELD_TASK:
            running._bit[ctxt->taskId] = false;
            waiting._bit[ctxt->taskId] = true;
            break;
        case SPURS_TASKSET_REQUEST_WAIT_SIGNAL:
            if (signalled._bit[ctxt->taskId] == false) {
                numNewlyReadyTasks--;
                running._bit[ctxt->taskId]   = false;
                waiting._bit[ctxt->taskId]   = true;
                signalled._bit[ctxt->taskId] = false;
                ready._bit[ctxt->taskId]     = false;
            }
            break;
        case SPURS_TASKSET_REQUEST_POLL:
            readyButNotRunning = ready & ~running;
            if (taskset->m.wkl_flag_wait_task < CELL_SPURS_MAX_TASK) {
                readyButNotRunning = readyButNotRunning & ~(u128::fromBit(taskset->m.wkl_flag_wait_task));
            }

            rc = readyButNotRunning != _0 ? 1 : 0;
            break;
        case SPURS_TASKSET_REQUEST_WAIT_WKL_FLAG:
            if (taskset->m.wkl_flag_wait_task == 0x81) {
                // A workload flag is already pending so consume it
                taskset->m.wkl_flag_wait_task = 0x80;
                rc                            = 0;
            } else if (taskset->m.wkl_flag_wait_task == 0x80) {
                // No tasks are waiting for the workload flag. Mark this task as waiting for the workload flag.
                taskset->m.wkl_flag_wait_task = ctxt->taskId;
                running._bit[ctxt->taskId]    = false;
                waiting._bit[ctxt->taskId]    = true;
                rc                            = 1;
                numNewlyReadyTasks--;
            } else {
                // Another task is already waiting for the workload signal
                rc = CELL_SPURS_TASK_ERROR_BUSY;
            }
            break;
        case SPURS_TASKSET_REQUEST_SELECT_TASK:
            readyButNotRunning = ready & ~running;
            if (taskset->m.wkl_flag_wait_task < CELL_SPURS_MAX_TASK) {
                readyButNotRunning = readyButNotRunning & ~(u128::fromBit(taskset->m.wkl_flag_wait_task));
            }

            // Select a task from the readyButNotRunning set to run. Start from the task after the last scheduled task to ensure fairness.
            for (selectedTaskId = taskset->m.last_scheduled_task + 1; selectedTaskId < 128; selectedTaskId++) {
                if (readyButNotRunning._bit[selectedTaskId]) {
                    break;
                }
            }

            if (selectedTaskId == 128) {
                for (selectedTaskId = 0; selectedTaskId < taskset->m.last_scheduled_task + 1; selectedTaskId++) {
                    if (readyButNotRunning._bit[selectedTaskId]) {
                        break;
                    }
                }

                if (selectedTaskId == taskset->m.last_scheduled_task + 1) {
                    selectedTaskId = CELL_SPURS_MAX_TASK;
                }
            }

            *taskId    = selectedTaskId;
            *isWaiting = waiting._bit[selectedTaskId < CELL_SPURS_MAX_TASK ? selectedTaskId : 0] ? 1 : 0;
            if (selectedTaskId != CELL_SPURS_MAX_TASK) {
                taskset->m.last_scheduled_task = selectedTaskId;
                running._bit[selectedTaskId]   = true;
                waiting._bit[selectedTaskId]   = false;
            }
            break;
        case SPURS_TASKSET_REQUEST_RECV_WKL_FLAG:
            if (taskset->m.wkl_flag_wait_task < CELL_SPURS_MAX_TASK) {
                // There is a task waiting for the workload flag
                taskset->m.wkl_flag_wait_task = 0x80;
                rc                            = 1;
                numNewlyReadyTasks++;
            } else {
                // No tasks are waiting for the workload flag
                taskset->m.wkl_flag_wait_task = 0x81;
                rc                            = 0;
            }
            break;
        default:
            spursHalt(spu);
            return CELL_OK;
        }

        taskset->m.pending_ready = _0;
        taskset->m.running       = running;
        taskset->m.waiting       = waiting;
        taskset->m.enabled       = enabled;
        taskset->m.signalled     = signalled;
        taskset->m.ready         = ready;
    } while (spursDma(spu, MFC_PUTLLC_CMD, ctxt->taskset.addr(), 0x2700/*LSA*/, 0x80/*size*/, 0/*tag*/) == false);

    // Increment the ready count of the workload by the number of tasks that have become ready
    do {
        spursDma(spu, MFC_GETLLAR_CMD, kernelCtxt->spurs.addr(), 0x100/*LSA*/, 0x80/*size*/, 0/*tag*/);
        auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x2D80 - offsetof(CellSpurs, m.wklState1));

        s32 readyCount  = kernelCtxt->wklCurrentId < CELL_SPURS_MAX_WORKLOAD ? spurs->m.wklReadyCount1[kernelCtxt->wklCurrentId].read_relaxed() : spurs->m.wklIdleSpuCountOrReadyCount2[kernelCtxt->wklCurrentId & 0x0F].read_relaxed();
        readyCount     += numNewlyReadyTasks;
        readyCount      = readyCount < 0 ? 0 : readyCount > 0xFF ? 0xFF : readyCount;

        if (kernelCtxt->wklCurrentId < CELL_SPURS_MAX_WORKLOAD) {
            spurs->m.wklReadyCount1[kernelCtxt->wklCurrentId].write_relaxed(readyCount);
        } else {
            spurs->m.wklIdleSpuCountOrReadyCount2[kernelCtxt->wklCurrentId & 0x0F].write_relaxed(readyCount);
        }
    } while (spursDma(spu, MFC_PUTLLC_CMD, kernelCtxt->spurs.addr(), 0x100/*LSA*/, 0x80/*size*/, 0/*tag*/) == false);

    return rc;
}

/// Process pollStatus received from the SPURS kernel
void spursTasksetProcessPollStatus(SPUThread & spu, u32 pollStatus) {
    if (pollStatus & CELL_SPURS_MODULE_POLL_STATUS_FLAG) {
        spursTasksetProcessRequest(spu, SPURS_TASKSET_REQUEST_RECV_WKL_FLAG, nullptr, nullptr);
    }
}

/// Check execution rights
bool spursTasksetPollStatus(SPUThread & spu) {
    u32 pollStatus;

    if (cellSpursModulePollStatus(spu, &pollStatus)) {
        return true;
    }

    spursTasksetProcessPollStatus(spu, pollStatus);
    return false;
}

/// Exit the Taskset PM
void spursTasksetExit(SPUThread & spu) {
    auto ctxt = vm::get_ptr<SpursTasksetContext>(spu.ls_offset + 0x2700);

    // Trace - STOP
    CellSpursTracePacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.header.tag = 0x54; // Its not clear what this tag means exactly but it seems similar to CELL_SPURS_TRACE_TAG_STOP
    pkt.data.stop  = SPURS_GUID_TASKSET_PM;
    cellSpursModulePutTrace(&pkt, ctxt->dmaTagId);

    // Not sure why this check exists. Perhaps to check for memory corruption.
    if (memcmp(ctxt->moduleId, "SPURSTASK MODULE", 16) != 0) {
        spursHalt(spu);
    }

    cellSpursModuleExit(spu);
}

/// Invoked when a task exits
void spursTasksetOnTaskExit(SPUThread & spu, u64 addr, u32 taskId, s32 exitCode, u64 args) {
    auto ctxt = vm::get_ptr<SpursTasksetContext>(spu.ls_offset + 0x2700);

    spursDma(spu, MFC_GET_CMD, addr & 0xFFFFFF80, 0x10000/*LSA*/, (addr & 0x7F) << 11/*size*/, 0);
    spursDmaWaitForCompletion(spu, 1);

    spu.GPR[3]._u64[1] = ctxt->taskset.addr();
    spu.GPR[4]._u32[3] = taskId;
    spu.GPR[5]._u32[3] = exitCode;
    spu.GPR[6]._u64[1] = args;
    spu.FastCall(0x10000);
}

/// Save the context of a task
s32 spursTasketSaveTaskContext(SPUThread & spu) {
    auto ctxt     = vm::get_ptr<SpursTasksetContext>(spu.ls_offset + 0x2700);
    auto taskInfo = vm::get_ptr<CellSpursTaskset::TaskInfo>(spu.ls_offset + 0x2780);

    spursDmaWaitForCompletion(spu, 0xFFFFFFFF);

    if (taskInfo->context_save_storage_and_alloc_ls_blocks == 0) {
        return CELL_SPURS_TASK_ERROR_STAT;
    }

    u32 allocLsBlocks = taskInfo->context_save_storage_and_alloc_ls_blocks & 0x7F;
    u32 lsBlocks      = 0;
    for (auto i = 0; i < 128; i++) {
        if (taskInfo->ls_pattern.u64[i < 64 ? 0 : 1] & (0x8000000000000000ull >> i)) {
            lsBlocks++;
        }
    }

    if (lsBlocks > allocLsBlocks) {
        return CELL_SPURS_TASK_ERROR_STAT;
    }

    // Make sure the stack is area is specified in the ls pattern
    for (auto i = (ctxt->savedContextSp.value()._u32[3]) >> 11; i < 128; i++) {
        if ((taskInfo->ls_pattern.u64[i < 64 ? 0 : 1] & (0x8000000000000000ull >> i)) == 0) {
            return CELL_SPURS_TASK_ERROR_STAT;
        }
    }

    // Get the processor context
    u128 r;
    spu.FPSCR.Read(r);
    ctxt->savedContextFpscr = r;
    spu.ReadChannel(r, SPU_RdEventMask);
    ctxt->savedSpuWriteEventMask = r._u32[3];
    spu.ReadChannel(r, MFC_RdTagMask);
    ctxt->savedWriteTagGroupQueryMask = r._u32[3];

    // Store the processor context
    u64 contextSaveStorage = taskInfo->context_save_storage_and_alloc_ls_blocks & 0xFFFFFFFFFFFFFF80ull;
    spursDma(spu, MFC_PUT_CMD, contextSaveStorage, 0x2C80/*LSA*/, 0x380/*size*/, ctxt->dmaTagId);

    // Save LS context
    for (auto i = 6; i < 128; i++) {
        bool shouldStore = taskInfo->ls_pattern.u64[i < 64 ? 0 : 1] & (0x8000000000000000ull >> i) ? true : false;
        if (shouldStore) {
            // TODO: Combine DMA requests for consecutive blocks into a single request
            spursDma(spu, MFC_PUT_CMD, contextSaveStorage + 0x400 + ((i - 6) << 11), CELL_SPURS_TASK_TOP + ((i - 6) << 11), 0x800/*size*/, ctxt->dmaTagId);
        }
    }

    spursDmaWaitForCompletion(spu, 1 << ctxt->dmaTagId);
    return CELL_OK;
}

/// Taskset dispatcher
void spursTasksetDispatch(SPUThread & spu) {
    auto ctxt    = vm::get_ptr<SpursTasksetContext>(spu.ls_offset + 0x2700);
    auto taskset = vm::get_ptr<CellSpursTaskset>(spu.ls_offset + 0x2700);

    u32 taskId;
    u32 isWaiting;
    spursTasksetProcessRequest(spu, SPURS_TASKSET_REQUEST_SELECT_TASK, &taskId, &isWaiting);
    if (taskId >= CELL_SPURS_MAX_TASK) {
        spursTasksetExit(spu);
        return;
    }

    ctxt->taskId = taskId;

    // DMA in the task info for the selected task
    spursDma(spu, MFC_GET_CMD, ctxt->taskset.addr() + offsetof(CellSpursTaskset, m.task_info[taskId]), 0x2780/*LSA*/, sizeof(CellSpursTaskset::TaskInfo), ctxt->dmaTagId);
    spursDmaWaitForCompletion(spu, 1 << ctxt->dmaTagId);
    auto taskInfo = vm::get_ptr<CellSpursTaskset::TaskInfo>(spu.ls_offset + 0x2780);
    auto elfAddr  = taskInfo->elf_addr.addr().value();
    taskInfo->elf_addr.set(taskInfo->elf_addr.addr() & 0xFFFFFFFFFFFFFFF8ull);

    // Trace - Task: Incident=dispatch
    CellSpursTracePacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.header.tag = CELL_SPURS_TRACE_TAG_TASK;
    pkt.data.task.incident = CELL_SPURS_TRACE_TASK_DISPATCH;
    pkt.data.task.taskId   = taskId;
    cellSpursModulePutTrace(&pkt, CELL_SPURS_KERNEL_DMA_TAG_ID);

    if (isWaiting == 0) {
        // If we reach here it means that the task is being started and not being resumed
        memset(vm::get_ptr<void>(spu.ls_offset + CELL_SPURS_TASK_TOP), 0, CELL_SPURS_TASK_BOTTOM - CELL_SPURS_TASK_TOP);
        ctxt->guidAddr = CELL_SPURS_TASK_TOP;

        u32 entryPoint;
        u32 lowestLoadAddr;
        if (spursTasksetLoadElf(spu, &entryPoint, &lowestLoadAddr, taskInfo->elf_addr.addr(), false) != CELL_OK) {
            spursHalt(spu);
            return;
        }

        spursDmaWaitForCompletion(spu, 1 << ctxt->dmaTagId);

        ctxt->savedContextLr  = u128::from32r(entryPoint);
        ctxt->guidAddr        = lowestLoadAddr;
        ctxt->tasksetMgmtAddr = 0x2700;
        ctxt->x2FC0           = 0;
        ctxt->taskExitCode    = isWaiting;
        ctxt->x2FD4           = elfAddr & 5; // TODO: Figure this out

        if ((elfAddr & 5) == 1) {
            spursDma(spu, MFC_GET_CMD, ctxt->taskset.addr() + offsetof(CellSpursTaskset2, m.task_exit_code[taskId]), 0x2FC0/*LSA*/, 0x10/*size*/, ctxt->dmaTagId);
        }

        // Trace - GUID
        memset(&pkt, 0, sizeof(pkt));
        pkt.header.tag = CELL_SPURS_TRACE_TAG_GUID;
        pkt.data.guid  = 0; // TODO: Put GUID of taskId here
        cellSpursModulePutTrace(&pkt, 0x1F);

        if (elfAddr & 2) { // TODO: Figure this out
            spu.SPU.Status.SetValue(SPU_STATUS_STOPPED_BY_STOP);
            spu.Stop();
            return;
        }

        spursTasksetStartTask(spu, taskInfo->args);
    } else {
        if (taskset->m.enable_clear_ls) {
            memset(vm::get_ptr<void>(spu.ls_offset + CELL_SPURS_TASK_TOP), 0, CELL_SPURS_TASK_BOTTOM - CELL_SPURS_TASK_TOP);
        }

        // If the entire LS is saved then there is no need to load the ELF as it will be be saved in the context save area as well
        if (taskInfo->ls_pattern.u64[1] != 0xFFFFFFFFFFFFFFFFull ||
            (taskInfo->ls_pattern.u64[0] | 0xFC00000000000000ull) != 0xFFFFFFFFFFFFFFFFull) {
            // Load the ELF
            u32 entryPoint;
            if (spursTasksetLoadElf(spu, &entryPoint, nullptr, taskInfo->elf_addr.addr(), true) != CELL_OK) {
                spursHalt(spu);
                return;
            }
        }

        // Load saved context from main memory to LS
        u64 contextSaveStorage = taskInfo->context_save_storage_and_alloc_ls_blocks & 0xFFFFFFFFFFFFFF80ull;
        spursDma(spu, MFC_GET_CMD, contextSaveStorage, 0x2C80/*LSA*/, 0x380/*size*/, ctxt->dmaTagId);
        for (auto i = 6; i < 128; i++) {
            bool shouldLoad = taskInfo->ls_pattern.u64[i < 64 ? 0 : 1] & (0x8000000000000000ull >> i) ? true : false;
            if (shouldLoad) {
                // TODO: Combine DMA requests for consecutive blocks into a single request
                spursDma(spu, MFC_GET_CMD, contextSaveStorage + 0x400 + ((i - 6) << 11), CELL_SPURS_TASK_TOP + ((i - 6) << 11), 0x800/*size*/, ctxt->dmaTagId);
            }
        }

        spursDmaWaitForCompletion(spu, 1 << ctxt->dmaTagId);

        // Restore saved registers
        spu.FPSCR.Write(ctxt->savedContextFpscr.value());
        spu.WriteChannel(MFC_WrTagMask, u128::from32r(ctxt->savedWriteTagGroupQueryMask));
        spu.WriteChannel(SPU_WrEventMask, u128::from32r(ctxt->savedSpuWriteEventMask));

        // Trace - GUID
        memset(&pkt, 0, sizeof(pkt));
        pkt.header.tag = CELL_SPURS_TRACE_TAG_GUID;
        pkt.data.guid  = 0; // TODO: Put GUID of taskId here
        cellSpursModulePutTrace(&pkt, 0x1F);

        if (elfAddr & 2) { // TODO: Figure this out
            spu.SPU.Status.SetValue(SPU_STATUS_STOPPED_BY_STOP);
            spu.Stop();
            return;
        }

        spu.GPR[3].clear();
        spursTasksetResumeTask(spu);
    }
}

/// Process a syscall request
s32 spursTasksetProcessSyscall(SPUThread & spu, u32 syscallNum, u32 args) {
    auto ctxt    = vm::get_ptr<SpursTasksetContext>(spu.ls_offset + 0x2700);
    auto taskset = vm::get_ptr<CellSpursTaskset>(spu.ls_offset + 0x2700);

    // If the 0x10 bit is set in syscallNum then its the 2nd version of the
    // syscall (e.g. cellSpursYield2 instead of cellSpursYield) and so don't wait
    // for DMA completion
    if ((syscallNum & 0x10) == 0) {
        spursDmaWaitForCompletion(spu, 0xFFFFFFFF);
    }

    s32 rc       = 0;
    u32 incident = 0;
    switch (syscallNum & 0x0F) {
    case CELL_SPURS_TASK_SYSCALL_EXIT:
        if (ctxt->x2FD4 == 4 || (ctxt->x2FC0 & 0xFFFFFFFF) != 0) { // TODO: Figure this out
            if (ctxt->x2FD4 != 4) {
                spursTasksetProcessRequest(spu, SPURS_TASKSET_REQUEST_DESTROY_TASK, nullptr, nullptr);
            }

            auto addr = ctxt->x2FD4 == 4 ? taskset->m.x78 : ctxt->x2FC0;
            auto args = ctxt->x2FD4 == 4 ? 0 : ctxt->x2FC8;
            spursTasksetOnTaskExit(spu, addr, ctxt->taskId, ctxt->taskExitCode, args);
        }

        incident = CELL_SPURS_TRACE_TASK_EXIT;
        break;
    case CELL_SPURS_TASK_SYSCALL_YIELD:
        if (spursTasksetPollStatus(spu) || spursTasksetProcessRequest(spu, SPURS_TASKSET_REQUEST_POLL, nullptr, nullptr)) {
            // If we reach here then it means that either another task can be scheduled or another workload can be scheduled
            // Save the context of the current task
            rc = spursTasketSaveTaskContext(spu);
            if (rc == CELL_OK) {
                spursTasksetProcessRequest(spu, SPURS_TASKSET_REQUEST_YIELD_TASK, nullptr, nullptr);
                incident = CELL_SPURS_TRACE_TASK_YIELD;
            }
        }
        break;
    case CELL_SPURS_TASK_SYSCALL_WAIT_SIGNAL:
        if (spursTasksetProcessRequest(spu, SPURS_TASKSET_REQUEST_POLL_SIGNAL, nullptr, nullptr) == 0) {
            rc = spursTasketSaveTaskContext(spu);
            if (rc == CELL_OK) {
                if (spursTasksetProcessRequest(spu, SPURS_TASKSET_REQUEST_WAIT_SIGNAL, nullptr, nullptr) == 0) {
                    incident = CELL_SPURS_TRACE_TASK_WAIT;
                }
            }
        }
        break;
    case CELL_SPURS_TASK_SYSCALL_POLL:
        rc  = spursTasksetPollStatus(spu) ? CELL_SPURS_TASK_POLL_FOUND_WORKLOAD : 0;
        rc |= spursTasksetProcessRequest(spu, SPURS_TASKSET_REQUEST_POLL, nullptr, nullptr) ? CELL_SPURS_TASK_POLL_FOUND_TASK : 0;
        break;
    case CELL_SPURS_TASK_SYSCALL_RECV_WKL_FLAG:
        if (args == 0) { // TODO: Figure this out
            spursHalt(spu);
        }

        if (spursTasksetPollStatus(spu) || spursTasksetProcessRequest(spu, SPURS_TASKSET_REQUEST_WAIT_WKL_FLAG, nullptr, nullptr) != 1) {
            rc = spursTasketSaveTaskContext(spu);
            if (rc == CELL_OK) {
                incident = CELL_SPURS_TRACE_TASK_WAIT;
            }
        }
        break;
    default:
        rc = CELL_SPURS_TASK_ERROR_NOSYS;
        break;
    }

    if (incident) {
        // Trace - TASK
        CellSpursTracePacket pkt;
        memset(&pkt, 0, sizeof(pkt));
        pkt.header.tag         = CELL_SPURS_TRACE_TAG_TASK;
        pkt.data.task.incident = incident;
        pkt.data.task.taskId   = ctxt->taskId;
        cellSpursModulePutTrace(&pkt, ctxt->dmaTagId);

        // Clear the GUID of the task
        memset(vm::get_ptr<void>(spu.ls_offset + ctxt->guidAddr), 0, 0x10);

        if (spursTasksetPollStatus(spu)) {
            spursTasksetExit(spu);
        } else {
            spursTasksetDispatch(spu);
        }
    }

    return rc;
}

/// Initialise the Taskset PM
void spursTasksetInit(SPUThread & spu, u32 pollStatus) {
    auto ctxt       = vm::get_ptr<SpursTasksetContext>(spu.ls_offset + 0x2700);
    auto kernelCtxt = vm::get_ptr<SpursKernelContext>(spu.ls_offset + 0x100);

    kernelCtxt->moduleId[0] = 'T';
    kernelCtxt->moduleId[1] = 'K';

    // Trace - START: Module='TKST'
    CellSpursTracePacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.header.tag = 0x52; // Its not clear what this tag means exactly but it seems similar to CELL_SPURS_TRACE_TAG_START
    memcpy(pkt.data.start.module, "TKST", 4);
    pkt.data.start.level = 2;
    pkt.data.start.ls    = 0xA00 >> 2;
    cellSpursModulePutTrace(&pkt, ctxt->dmaTagId);

    spursTasksetProcessPollStatus(spu, pollStatus);
}

/// Load an ELF
s32 spursTasksetLoadElf(SPUThread & spu, u32 * entryPoint, u32 * lowestLoadAddr, u64 elfAddr, bool skipWriteableSegments) {
    if (elfAddr == 0 || (elfAddr & 0x0F) != 0) {
        return CELL_SPURS_TASK_ERROR_INVAL;
    }

    vfsStreamMemory         stream(elfAddr);
    loader::handlers::elf32 loader;
    auto rc = loader.init(stream);
    if (rc != loader::handler::ok) {
        return CELL_SPURS_TASK_ERROR_NOEXEC;
    }

    u32 _lowestLoadAddr = CELL_SPURS_TASK_BOTTOM;
    for (auto & phdr : loader.m_phdrs) {
        if (phdr.data_be.p_paddr >= CELL_SPURS_TASK_BOTTOM) {
            break;
        }

        if (phdr.data_be.p_type == 1/*PT_LOAD*/) {
            if (skipWriteableSegments == false || (phdr.data_be.p_flags & 2/*PF_W*/) == 0) {
                if (phdr.data_be.p_vaddr < CELL_SPURS_TASK_TOP ||
                    phdr.data_be.p_vaddr + phdr.data_be.p_memsz > CELL_SPURS_TASK_BOTTOM) {
                    return CELL_SPURS_TASK_ERROR_FAULT;
                }

                _lowestLoadAddr = _lowestLoadAddr > phdr.data_be.p_vaddr ? phdr.data_be.p_vaddr : _lowestLoadAddr;
            }
        }
    }

    loader.load_data(spu.ls_offset, skipWriteableSegments);
    *entryPoint = loader.m_ehdr.data_be.e_entry;
    if (*lowestLoadAddr) {
        *lowestLoadAddr = _lowestLoadAddr;
    }

    return CELL_OK;
}
