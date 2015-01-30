#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/lv2/sys_lwmutex.h"
#include "Emu/SysCalls/lv2/sys_lwcond.h"
#include "Emu/SysCalls/lv2/sys_spu.h"
#include "Emu/SysCalls/Modules/cellSpurs.h"

//
// SPURS utility functions
//
void cellSpursModulePutTrace(CellSpursTracePacket * packet, unsigned tag);
u32 cellSpursModulePollStatus(SPUThread & spu, u32 * status);
void cellSpursModuleExit(SPUThread & spu);

bool spursDma(SPUThread & spu, u32 cmd, u64 ea, u32 lsa, u32 size, u32 tag);
u32 spursDmaGetCompletionStatus(SPUThread & spu, u32 tagMask);
u32 spursDmaWaitForCompletion(SPUThread & spu, u32 tagMask, bool waitForAll = true);
void spursHalt();

//
// SPURS Kernel functions
//
bool spursKernel1SelectWorkload(SPUThread & spu);
bool spursKernel2SelectWorkload(SPUThread & spu);

//
// SPURS system service workload functions
//
void spursSysServiceCleanupAfterPreemption(SPUThread & spu, SpursKernelMgmtData * mgmt);
void spursSysServiceUpdateTraceCount(SPUThread & spu, SpursKernelMgmtData * mgmt);
void spursSysServiceUpdateTrace(SPUThread & spu, SpursKernelMgmtData * mgmt, u32 arg2, u32 arg3, u32 arg4);
void spursSysServiceUpdateEvent(SPUThread & spu, SpursKernelMgmtData * mgmt, u32 wklShutdownBitSet);
void spursSysServiceUpdateWorkload(SPUThread & spu, SpursKernelMgmtData * mgmt);
void spursSysServiceProcessMessages(SPUThread & spu, SpursKernelMgmtData * mgmt);
void spursSysServiceWaitOrExit(SPUThread & spu, SpursKernelMgmtData * mgmt);
void spursSysServiceWorkloadMain(SPUThread & spu, u32 pollStatus);
bool spursSysServiceWorkloadEntry(SPUThread & spu);

//
// SPURS taskset policy module functions
//
bool spursTasksetProcessRequest(SPUThread & spu, s32 request, u32 * taskId, u32 * isWaiting);
void spursTasksetExit(SPUThread & spu);
void spursTasksetDispatch(SPUThread & spu);
void spursTasksetProcessPollStatus(SPUThread & spu, u32 pollStatus);
bool spursTasksetShouldYield(SPUThread & spu);
void spursTasksetInit(SPUThread & spu, u32 pollStatus);
void spursTasksetResumeTask(SPUThread & spu);
bool spursTasksetEntry(SPUThread & spu);

extern Module *cellSpurs;

//////////////////////////////////////////////////////////////////////////////
// SPURS utility functions
//////////////////////////////////////////////////////////////////////////////

/// Output trace information
void cellSpursModulePutTrace(CellSpursTracePacket * packet, unsigned tag) {
    // TODO: Implement this
}

/// Check for execution right requests
u32 cellSpursModulePollStatus(SPUThread & spu, u32 * status) {
    auto mgmt = vm::get_ptr<SpursKernelMgmtData>(spu.ls_offset + 0x100);

    spu.GPR[3]._u32[3] = 1;
    if (mgmt->spurs->m.flags1 & SF1_32_WORKLOADS) {
        spursKernel2SelectWorkload(spu);
    } else {
        spursKernel1SelectWorkload(spu);
    }

    auto result = spu.GPR[3]._u64[1];
    if (status) {
        *status = (u32)result;
    }

    u32 wklId = result >> 32;
    return wklId == mgmt->wklCurrentId ? 0 : 1;
}

/// Exit current workload
void cellSpursModuleExit(SPUThread & spu) {
    auto mgmt = vm::get_ptr<SpursKernelMgmtData>(spu.ls_offset + 0x100);
    spu.SetBranch(mgmt->yieldToKernelAddr);
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
        return rv._u32[3] ? true : false;
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

// Halt the SPU
void spursHalt(SPUThread & spu) {
    spu.SPU.Status.SetValue(SPU_STATUS_STOPPED_BY_HALT);
    spu.Stop();
}

//////////////////////////////////////////////////////////////////////////////
// SPURS kernel functions
//////////////////////////////////////////////////////////////////////////////

/// Select a workload to run
bool spursKernel1SelectWorkload(SPUThread & spu) {
    auto mgmt = vm::get_ptr<SpursKernelMgmtData>(spu.ls_offset + 0x100);

    // The first and only argument to this function is a boolean that is set to false if the function
    // is called by the SPURS kernel and set to true if called by cellSpursModulePollStatus.
    // If the first argument is true then the shared data is not updated with the result.
    const auto isPoll = spu.GPR[3]._u32[3];

    u32 wklSelectedId;
    u32 pollStatus;

    do {
        // DMA and lock the first 0x80 bytes of spurs
        spursDma(spu, MFC_GETLLAR_CMD, mgmt->spurs.addr(), 0x100/*LSA*/, 0x80/*size*/, 0/*tag*/);
        auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x100);

        // Calculate the contention (number of SPUs used) for each workload
        u8 contention[CELL_SPURS_MAX_WORKLOAD];
        u8 pendingContention[CELL_SPURS_MAX_WORKLOAD];
        for (auto i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
            contention[i] = spurs->m.wklCurrentContention[i] - mgmt->wklLocContention[i];

            // If this is a poll request then the number of SPUs pending to context switch is also added to the contention presumably
            // to prevent unnecessary jumps to the kernel
            if (isPoll) {
                pendingContention[i] = spurs->m.wklPendingContention[i] - mgmt->wklLocPendingContention[i];
                if (i != mgmt->wklCurrentId) {
                    contention[i] += pendingContention[i];
                }
            }
        }

        wklSelectedId = CELL_SPURS_SYS_SERVICE_WORKLOAD_ID;
        pollStatus    = 0;

        // The system service workload has the highest priority. Select the system service workload if
        // the system service message bit for this SPU is set.
        if (spurs->m.sysSrvMessage.read_relaxed() & (1 << mgmt->spuNum)) {
            mgmt->spuIdling = 0;
            if (!isPoll || mgmt->wklCurrentId == CELL_SPURS_SYS_SERVICE_WORKLOAD_ID) {
                // Clear the message bit
                spurs->m.sysSrvMessage.write_relaxed(spurs->m.sysSrvMessage.read_relaxed() & ~(1 << mgmt->spuNum));
            }
        } else {
            // Caclulate the scheduling weight for each workload
            u16 maxWeight = 0;
            for (auto i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
                u16 runnable     = mgmt->wklRunnable1 & (0x8000 >> i);
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
                if (runnable && mgmt->priority[i] != 0 && spurs->m.wklMaxContention[i].read_relaxed() > contention[i]) {
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
                        weight     |= (u16)(mgmt->priority[i] & 0x7F) << 16;
                        weight     |= i == mgmt->wklCurrentId ? 0x80 : 0x00;
                        weight     |= (contention[i] > 0 && spurs->m.wklMinContention[i] > contention[i]) ? 0x40 : 0x00;
                        weight     |= ((CELL_SPURS_MAX_SPU - contention[i]) & 0x0F) << 2;
                        weight     |= mgmt->wklUniqueId[i] == mgmt->wklCurrentId ? 0x02 : 0x00;
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
            mgmt->spuIdling = wklSelectedId == CELL_SPURS_SYS_SERVICE_WORKLOAD_ID ? 1 : 0;

            if (!isPoll || wklSelectedId == mgmt->wklCurrentId) {
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
                mgmt->wklLocContention[i]        = 0;
                mgmt->wklLocPendingContention[i] = 0;
            }

            if (wklSelectedId != CELL_SPURS_SYS_SERVICE_WORKLOAD_ID) {
                mgmt->wklLocContention[wklSelectedId] = 1;
            }

            mgmt->wklCurrentId = wklSelectedId;
        } else if (wklSelectedId != mgmt->wklCurrentId) {
            // Not called by kernel but a context switch is required
            // Increment the pending contention for the selected workload
            if (wklSelectedId != CELL_SPURS_SYS_SERVICE_WORKLOAD_ID) {
                pendingContention[wklSelectedId]++;
            }

            for (auto i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
                spurs->m.wklPendingContention[i] = pendingContention[i];
                mgmt->wklLocPendingContention[i] = 0;
            }

            if (wklSelectedId != CELL_SPURS_SYS_SERVICE_WORKLOAD_ID) {
                mgmt->wklLocPendingContention[wklSelectedId] = 1;
            }
        }
    } while (spursDma(spu, MFC_PUTLLC_CMD, mgmt->spurs.addr(), 0x100/*LSA*/, 0x80/*size*/, 0/*tag*/) == false);

    u64 result          = (u64)wklSelectedId << 32;
    result             |= pollStatus;
    spu.GPR[3]._u64[1]  = result;
    return true;
}

/// Select a workload to run
bool spursKernel2SelectWorkload(SPUThread & spu) {
    auto mgmt = vm::get_ptr<SpursKernelMgmtData>(spu.ls_offset + 0x100);

    // The first and only argument to this function is a boolean that is set to false if the function
    // is called by the SPURS kernel and set to true if called by cellSpursModulePollStatus.
    // If the first argument is true then the shared data is not updated with the result.
    const auto isPoll = spu.GPR[3]._u32[3];

    u32 wklSelectedId;
    u32 pollStatus;

    do {
        // DMA and lock the first 0x80 bytes of spurs
        spursDma(spu, MFC_GETLLAR_CMD, mgmt->spurs.addr(), 0x100/*LSA*/, 0x80/*size*/, 0/*tag*/);
        auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x100);

        // Calculate the contention (number of SPUs used) for each workload
        u8 contention[CELL_SPURS_MAX_WORKLOAD2];
        u8 pendingContention[CELL_SPURS_MAX_WORKLOAD2];
        for (auto i = 0; i < CELL_SPURS_MAX_WORKLOAD2; i++) {
            contention[i] = spurs->m.wklCurrentContention[i & 0x0F] - mgmt->wklLocContention[i & 0x0F];
            contention[i] = i < CELL_SPURS_MAX_WORKLOAD ? contention[i] & 0x0F : contention[i] >> 4;

            // If this is a poll request then the number of SPUs pending to context switch is also added to the contention presumably
            // to prevent unnecessary jumps to the kernel
            if (isPoll) {
                pendingContention[i] = spurs->m.wklPendingContention[i & 0x0F] - mgmt->wklLocPendingContention[i & 0x0F];
                pendingContention[i] = i < CELL_SPURS_MAX_WORKLOAD ? pendingContention[i] & 0x0F : pendingContention[i] >> 4;
                if (i != mgmt->wklCurrentId) {
                    contention[i] += pendingContention[i];
                }
            }
        }

        wklSelectedId = CELL_SPURS_SYS_SERVICE_WORKLOAD_ID;
        pollStatus    = 0;

        // The system service workload has the highest priority. Select the system service workload if
        // the system service message bit for this SPU is set.
        if (spurs->m.sysSrvMessage.read_relaxed() & (1 << mgmt->spuNum)) {
            // Not sure what this does. Possibly Mark the SPU as in use.
            mgmt->spuIdling = 0;
            if (!isPoll || mgmt->wklCurrentId == CELL_SPURS_SYS_SERVICE_WORKLOAD_ID) {
                // Clear the message bit
                spurs->m.sysSrvMessage.write_relaxed(spurs->m.sysSrvMessage.read_relaxed() & ~(1 << mgmt->spuNum));
            }
        } else {
            // Caclulate the scheduling weight for each workload
            u8 maxWeight = 0;
            for (auto i = 0; i < CELL_SPURS_MAX_WORKLOAD2; i++) {
                auto j           = i & 0x0F;
                u16 runnable      = i < CELL_SPURS_MAX_WORKLOAD ? mgmt->wklRunnable1 & (0x8000 >> j) : mgmt->wklRunnable2 & (0x8000 >> j);
                u8  priority      = i < CELL_SPURS_MAX_WORKLOAD ? mgmt->priority[j] & 0x0F : mgmt->priority[j] >> 4;
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
                        if (mgmt->wklCurrentId == i) {
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
            mgmt->spuIdling = wklSelectedId == CELL_SPURS_SYS_SERVICE_WORKLOAD_ID ? 1 : 0;

            if (!isPoll || wklSelectedId == mgmt->wklCurrentId) {
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
                mgmt->wklLocContention[i]        = 0;
                mgmt->wklLocPendingContention[i] = 0;
            }

            mgmt->wklLocContention[wklSelectedId & 0x0F] = wklSelectedId < CELL_SPURS_MAX_WORKLOAD ? 0x01 : wklSelectedId < CELL_SPURS_MAX_WORKLOAD2 ? 0x10 : 0;
            mgmt->wklCurrentId = wklSelectedId;
        } else if (wklSelectedId != mgmt->wklCurrentId) {
            // Not called by kernel but a context switch is required
            // Increment the pending contention for the selected workload
            if (wklSelectedId != CELL_SPURS_SYS_SERVICE_WORKLOAD_ID) {
                pendingContention[wklSelectedId]++;
            }

            for (auto i = 0; i < (CELL_SPURS_MAX_WORKLOAD2 >> 1); i++) {
                spurs->m.wklPendingContention[i] = pendingContention[i] | (pendingContention[i + 0x10] << 4);
                mgmt->wklLocPendingContention[i] = 0;
            }

            mgmt->wklLocPendingContention[wklSelectedId & 0x0F] = wklSelectedId < CELL_SPURS_MAX_WORKLOAD ? 0x01 : wklSelectedId < CELL_SPURS_MAX_WORKLOAD2 ? 0x10 : 0;
        }
    } while (spursDma(spu, MFC_PUTLLC_CMD, mgmt->spurs.addr(), 0x100/*LSA*/, 0x80/*size*/, 0/*tag*/) == false);

    u64 result          = (u64)wklSelectedId << 32;
    result             |= pollStatus;
    spu.GPR[3]._u64[1]  = result;
    return true;
}

/// SPURS kernel main
bool spursKernelMain(SPUThread & spu) {
    SpursKernelMgmtData * mgmt = vm::get_ptr<SpursKernelMgmtData>(spu.ls_offset + 0x100);

    bool isKernel2;
    u32  pollStatus;
    if (spu.PC == CELL_SPURS_KERNEL1_ENTRY_ADDR || spu.PC == CELL_SPURS_KERNEL2_ENTRY_ADDR) {
        // Entry point of SPURS kernel
        // Save arguments
        mgmt->spuNum = spu.GPR[3]._u32[3];
        mgmt->spurs.set(spu.GPR[4]._u64[1]);

        isKernel2 = mgmt->spurs->m.flags1 & SF1_32_WORKLOADS ? true : false;

        memset(mgmt, 0, sizeof(SpursKernelMgmtData));

        // Initialise the SPURS management area to its initial values
        mgmt->dmaTagId           = CELL_SPURS_KERNEL_DMA_TAG_ID;
        mgmt->wklCurrentUniqueId = 0x20;
        mgmt->wklCurrentId       = CELL_SPURS_SYS_SERVICE_WORKLOAD_ID;
        mgmt->yieldToKernelAddr  = isKernel2 ? CELL_SPURS_KERNEL2_YIELD_ADDR : CELL_SPURS_KERNEL1_YIELD_ADDR;
        mgmt->selectWorkloadAddr = isKernel2 ? CELL_SPURS_KERNEL2_SELECT_WORKLOAD_ADDR : CELL_SPURS_KERNEL1_SELECT_WORKLOAD_ADDR;
        if (!isKernel2) {
            mgmt->x1F0    = 0xF0020000;
            mgmt->x200    = 0x20000;
            mgmt->guid[0] = 0x423A3A02;
            mgmt->guid[1] = 0x43F43A82;
            mgmt->guid[2] = 0x43F26502;
            mgmt->guid[3] = 0x420EB382;
        } else {
            mgmt->guid[0] = 0x43A08402;
            mgmt->guid[1] = 0x43FB0A82;
            mgmt->guid[2] = 0x435E9302;
            mgmt->guid[3] = 0x43A3C982;
        }

        // Register SPURS kernel HLE functions
        spu.UnregisterHleFunctions(0, 0x40000); // TODO: use a symbolic constant
        spu.RegisterHleFunction(isKernel2 ? CELL_SPURS_KERNEL2_ENTRY_ADDR : CELL_SPURS_KERNEL1_ENTRY_ADDR, spursKernelMain);
        spu.RegisterHleFunction(mgmt->yieldToKernelAddr, spursKernelMain);
        spu.RegisterHleFunction(mgmt->selectWorkloadAddr, isKernel2 ? spursKernel2SelectWorkload : spursKernel1SelectWorkload);

        // DMA in the system service workload info
        spursDma(spu, MFC_GET_CMD, mgmt->spurs.addr() + offsetof(CellSpurs, m.wklInfoSysSrv), 0x3FFE0/*LSA*/, 0x20/*size*/, CELL_SPURS_KERNEL_DMA_TAG_ID/*tag*/);
        spursDmaWaitForCompletion(spu, 0x80000000);
        pollStatus = 0;
    } else if (spu.PC == mgmt->yieldToKernelAddr) {
        isKernel2 = mgmt->spurs->m.flags1 & SF1_32_WORKLOADS ? true : false;

        // Select next workload to run
        spu.GPR[3].clear();
        if (isKernel2) {
            spursKernel2SelectWorkload(spu);
        } else {
            spursKernel1SelectWorkload(spu);
        }

        pollStatus = (u32)(spu.GPR[3]._u64[1]);
        auto wid   = (u32)(spu.GPR[3]._u64[1] >> 32);

        // DMA in the workload info for the selected workload
        auto wklInfoOffset =  wid < CELL_SPURS_MAX_WORKLOAD ? offsetof(CellSpurs, m.wklInfo1[wid]) :
                                                              wid < CELL_SPURS_MAX_WORKLOAD2 && isKernel2 ? offsetof(CellSpurs, m.wklInfo2[wid & 0xf]) :
                                                                                                            offsetof(CellSpurs, m.wklInfoSysSrv);
        spursDma(spu, MFC_GET_CMD, mgmt->spurs.addr() + wklInfoOffset, 0x3FFE0/*LSA*/, 0x20/*size*/, CELL_SPURS_KERNEL_DMA_TAG_ID);
        spursDmaWaitForCompletion(spu, 0x80000000);
    } else {
        assert(0);
    }

    auto wklInfo = vm::get_ptr<CellSpurs::WorkloadInfo>(spu.ls_offset + 0x3FFE0);
    if (mgmt->wklCurrentAddr != wklInfo->addr) {
        switch (wklInfo->addr.addr()) {
        case SPURS_IMG_ADDR_SYS_SRV_WORKLOAD:
            spu.RegisterHleFunction(0xA00, spursSysServiceWorkloadEntry);
            break;
        case SPURS_IMG_ADDR_TASKSET_PM:
            spu.RegisterHleFunction(0xA00, spursTasksetEntry);
            break;
        default:
            spursDma(spu, MFC_GET_CMD, wklInfo-> addr.addr(), 0xA00/*LSA*/, wklInfo->size, CELL_SPURS_KERNEL_DMA_TAG_ID);
            spursDmaWaitForCompletion(spu, 0x80000000);
            break;
        }

        mgmt->wklCurrentAddr     = wklInfo->addr;
        mgmt->wklCurrentUniqueId = wklInfo->uniqueId.read_relaxed();
    }

    if (!isKernel2) {
        mgmt->moduleId[0] = 0;
        mgmt->moduleId[1] = 0;
    }

    // Run workload
    spu.GPR[0]._u32[3] = mgmt->yieldToKernelAddr;
    spu.GPR[1]._u32[3] = 0x3FFB0;
    spu.GPR[3]._u32[3] = 0x100;
    spu.GPR[4]._u64[1] = wklInfo->arg;
    spu.GPR[5]._u32[3] = pollStatus;
    spu.SetBranch(0xA00);
    return false;
}

//////////////////////////////////////////////////////////////////////////////
// SPURS system workload functions
//////////////////////////////////////////////////////////////////////////////

/// Restore scheduling parameters after a workload has been preempted by the system service workload
void spursSysServiceCleanupAfterPreemption(SPUThread & spu, SpursKernelMgmtData * mgmt) {
    u8 wklId;

    do {
        spursDma(spu, MFC_GETLLAR_CMD, mgmt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/);
        auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x2D80 - offsetof(CellSpurs, m.wklState1));

        if (spurs->m.sysSrvWorkload[mgmt->spuNum] == 0xFF) {
            return;
        }

        wklId = spurs->m.sysSrvWorkload[mgmt->spuNum];
        spurs->m.sysSrvWorkload[mgmt->spuNum] = 0xFF;
    } while (spursDma(spu, MFC_PUTLLC_CMD, mgmt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/) == false);

    spursSysServiceUpdateWorkload(spu, mgmt);

    do {
        spursDma(spu, MFC_GETLLAR_CMD, mgmt->spurs.addr(), 0x100/*LSA*/, 0x80/*size*/, 0/*tag*/);
        auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x100);

        if (wklId >= CELL_SPURS_MAX_WORKLOAD) {
            spurs->m.wklCurrentContention[wklId & 0x0F] -= 0x10;
            spurs->m.wklReadyCount1[wklId & 0x0F].write_relaxed(spurs->m.wklReadyCount1[wklId & 0x0F].read_relaxed() - 1);
        } else {
            spurs->m.wklCurrentContention[wklId & 0x0F] -= 0x01;
            spurs->m.wklIdleSpuCountOrReadyCount2[wklId & 0x0F].write_relaxed(spurs->m.wklIdleSpuCountOrReadyCount2[wklId & 0x0F].read_relaxed() - 1);
        }
    } while (spursDma(spu, MFC_PUTLLC_CMD, mgmt->spurs.addr(), 0x100/*LSA*/, 0x80/*size*/, 0/*tag*/) == false);

    // Set the current workload id to the id of the pre-empted workload since cellSpursModulePutTrace
    // uses the current worload id to determine the workload to which the trace belongs
    auto wklIdSaved    = mgmt->wklCurrentId;
    mgmt->wklCurrentId = wklId;

    // Trace - STOP: GUID
    CellSpursTracePacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.header.tag = CELL_SPURS_TRACE_TAG_STOP;
    pkt.data.stop  = SPURS_GUID_SYS_WKL;
    cellSpursModulePutTrace(&pkt, mgmt->dmaTagId);

    mgmt->wklCurrentId = wklIdSaved;
}

/// Update the trace count for this SPU in CellSpurs
void spursSysServiceUpdateTraceCount(SPUThread & spu, SpursKernelMgmtData * mgmt) {
    if (mgmt->traceBuffer) {
        auto traceInfo = vm::ptr<CellSpursTraceInfo>::make((u32)(mgmt->traceBuffer - (mgmt->spurs->m.traceStartIndex[mgmt->spuNum] << 4)));
        traceInfo->count[mgmt->spuNum] = mgmt->traceMsgCount;
    }
}

/// Update trace control in SPU from CellSpurs
void spursSysServiceUpdateTrace(SPUThread & spu, SpursKernelMgmtData * mgmt, u32 arg2, u32 arg3, u32 arg4) {
    bool notify;

    u8 sysSrvMsgUpdateTrace;
    do {
        spursDma(spu, MFC_GETLLAR_CMD, mgmt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/);
        auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x2D80 - offsetof(CellSpurs, m.wklState1));

        sysSrvMsgUpdateTrace           = spurs->m.sysSrvMsgUpdateTrace;
        spurs->m.sysSrvMsgUpdateTrace &= ~(1 << mgmt->spuNum);
        spurs->m.xCC                  &= ~(1 << mgmt->spuNum);
        spurs->m.xCC                  |= arg2 << mgmt->spuNum;

        notify = false;
        if (((sysSrvMsgUpdateTrace & (1 << mgmt->spuNum)) != 0) && (spurs->m.sysSrvMsgUpdateTrace == 0) && (spurs->m.xCD != 0)) {
            spurs->m.xCD = 0;
            notify       = true;
        }

        if (arg4 && spurs->m.xCD != 0) {
            spurs->m.xCD = 0;
            notify       = true;
        }
    } while (spursDma(spu, MFC_PUTLLC_CMD, mgmt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/) == false);

    // Get trace parameters from CellSpurs and store them in the LS
    if (((sysSrvMsgUpdateTrace & (1 << mgmt->spuNum)) != 0) || (arg3 != 0)) {
        spursDma(spu, MFC_GETLLAR_CMD, mgmt->spurs.addr() + offsetof(CellSpurs, m.traceBuffer), 0x80/*LSA*/, 0x80/*size*/, 0/*tag*/);
        auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x80 - offsetof(CellSpurs, m.traceBuffer));

        if (mgmt->traceMsgCount != 0xFF || spurs->m.traceBuffer.addr() == 0) {
            spursSysServiceUpdateTraceCount(spu, mgmt);
        } else {
            spursDma(spu, MFC_GET_CMD, spurs->m.traceBuffer.addr() & 0xFFFFFFFC, 0x2C00/*LSA*/, 0x80/*size*/, mgmt->dmaTagId);
            auto traceBuffer    = vm::get_ptr<CellSpursTraceInfo>(spu.ls_offset + 0x2C00);
            mgmt->traceMsgCount = traceBuffer->count[mgmt->spuNum];
        }

        mgmt->traceBuffer   = spurs->m.traceBuffer.addr() + (spurs->m.traceStartIndex[mgmt->spuNum] << 4);
        mgmt->traceMaxCount = spurs->m.traceStartIndex[1] - spurs->m.traceStartIndex[0];
        if (mgmt->traceBuffer == 0) {
            mgmt->traceMsgCount = 0;
        }
    }

    if (notify) {
        auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x2D80 - offsetof(CellSpurs, m.wklState1));
        sys_spu_thread_send_event(spu, spurs->m.spuPort, 2, 0);
    }
}

/// Update events in CellSpurs
void spursSysServiceUpdateEvent(SPUThread & spu, SpursKernelMgmtData * mgmt, u32 wklShutdownBitSet) {
    // Mark the workloads in wklShutdownBitSet as completed and also generate a bit set of the completed
    // workloads that have a shutdown completion hook registered
    u32 wklNotifyBitSet;
    u8  spuPort;
    do {
        spursDma(spu, MFC_GETLLAR_CMD, mgmt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/);
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
    } while (spursDma(spu, MFC_PUTLLC_CMD, mgmt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/) == false);

    if (wklNotifyBitSet) {
        // TODO: sys_spu_thread_send_event(spuPort, 0, wklNotifyMask);
    }
}

/// Update workload information in the SPU from CellSpurs
void spursSysServiceUpdateWorkload(SPUThread & spu, SpursKernelMgmtData * mgmt) {
    auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x100);
    spursDma(spu, MFC_GET_CMD, mgmt->spurs.addr() + offsetof(CellSpurs, m.wklInfo1), 0x30000/*LSA*/, 0x200/*size*/, CELL_SPURS_KERNEL_DMA_TAG_ID);
    if (spurs->m.flags1 & SF1_32_WORKLOADS) {
        spursDma(spu, MFC_GET_CMD, mgmt->spurs.addr() + offsetof(CellSpurs, m.wklInfo2), 0x30200/*LSA*/, 0x200/*size*/, CELL_SPURS_KERNEL_DMA_TAG_ID);
    }

    u32 wklShutdownBitSet = 0;
    mgmt->wklRunnable1    = 0;
    mgmt->wklRunnable2    = 0;
    for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
        auto wklInfo1 = vm::get_ptr<CellSpurs::WorkloadInfo>(spu.ls_offset + 0x30000);

        // Copy the priority of the workload for this SPU and its unique id to the LS
        mgmt->priority[i]    = wklInfo1[i].priority[mgmt->spuNum] == 0 ? 0 : 0x10 - wklInfo1[i].priority[mgmt->spuNum];
        mgmt->wklUniqueId[i] = wklInfo1[i].uniqueId.read_relaxed();

        if (spurs->m.flags1 & SF1_32_WORKLOADS) {
            auto wklInfo2 = vm::get_ptr<CellSpurs::WorkloadInfo>(spu.ls_offset + 0x30200);

            // Copy the priority of the workload for this SPU to the LS
            if (wklInfo2[i].priority[mgmt->spuNum]) {
                mgmt->priority[i] |= (0x10 - wklInfo2[i].priority[mgmt->spuNum]) << 4;
            }
        }
    }

    do {
        spursDma(spu, MFC_GETLLAR_CMD, mgmt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/);
        spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x2D80 - offsetof(CellSpurs, m.wklState1));

        for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
            // Update workload status and runnable flag based on the workload state
            auto wklStatus = spurs->m.wklStatus1[i];
            if (spurs->m.wklState1[i].read_relaxed() == SPURS_WKL_STATE_RUNNABLE) {
                spurs->m.wklStatus1[i] |= 1 << mgmt->spuNum;
                mgmt->wklRunnable1     |= 0x8000 >> i;
            } else {
                spurs->m.wklStatus1[i] &= ~(1 << mgmt->spuNum);
            }

            // If the workload is shutting down and if this is the last SPU from which it is being removed then
            // add it to the shutdown bit set
            if (spurs->m.wklState1[i].read_relaxed() == SPURS_WKL_STATE_SHUTTING_DOWN) {
                if (((wklStatus & (1 << mgmt->spuNum)) != 0) && (spurs->m.wklStatus1[i] == 0)) {
                    spurs->m.wklState1[i].write_relaxed(SPURS_WKL_STATE_REMOVABLE);
                    wklShutdownBitSet |= 0x80000000u >> i;
                }
            }

            if (spurs->m.flags1 & SF1_32_WORKLOADS) {
                // Update workload status and runnable flag based on the workload state
                wklStatus = spurs->m.wklStatus2[i];
                if (spurs->m.wklState2[i].read_relaxed() == SPURS_WKL_STATE_RUNNABLE) {
                    spurs->m.wklStatus2[i] |= 1 << mgmt->spuNum;
                    mgmt->wklRunnable2     |= 0x8000 >> i;
                } else {
                    spurs->m.wklStatus2[i] &= ~(1 << mgmt->spuNum);
                }

                // If the workload is shutting down and if this is the last SPU from which it is being removed then
                // add it to the shutdown bit set
                if (spurs->m.wklState2[i].read_relaxed() == SPURS_WKL_STATE_SHUTTING_DOWN) {
                    if (((wklStatus & (1 << mgmt->spuNum)) != 0) && (spurs->m.wklStatus2[i] == 0)) {
                        spurs->m.wklState2[i].write_relaxed(SPURS_WKL_STATE_REMOVABLE);
                        wklShutdownBitSet |= 0x8000 >> i;
                    }
                }
            }
        }
    } while (spursDma(spu, MFC_PUTLLC_CMD, mgmt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/) == false);

    if (wklShutdownBitSet) {
        spursSysServiceUpdateEvent(spu, mgmt, wklShutdownBitSet);
    }
}

/// Process any messages
void spursSysServiceProcessMessages(SPUThread & spu, SpursKernelMgmtData * mgmt) {
    bool updateTrace    = false;
    bool updateWorkload = false;
    bool terminate      = false;

    do {
        spursDma(spu, MFC_GETLLAR_CMD, mgmt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/);
        auto spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x2D80 - offsetof(CellSpurs, m.wklState1));

        // Terminate request
        if (spurs->m.sysSrvMsgTerminate & (1 << mgmt->spuNum)) {
            spurs->m.sysSrvOnSpu &= ~(1 << mgmt->spuNum);
            terminate = true;
        }

        // Update workload message
        if (spurs->m.sysSrvMsgUpdateWorkload.read_relaxed() & (1 << mgmt->spuNum)) {
            spurs->m.sysSrvMsgUpdateWorkload &= ~(1 << mgmt->spuNum);
            updateWorkload = true;
        }

        // Update trace message
        if (spurs->m.sysSrvMsgUpdateTrace & (1 << mgmt->spuNum)) {
            updateTrace = true;
        }
    } while (spursDma(spu, MFC_PUTLLC_CMD, mgmt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/) == false);

    // Process update workload message
    if (updateWorkload) {
        spursSysServiceUpdateWorkload(spu, mgmt);
    }

    // Process update trace message
    if (updateTrace) {
        spursSysServiceUpdateTrace(spu, mgmt, 1, 0, 0);
    }

    // Process terminate request
    if (terminate) {
        // TODO: Rest of the terminate processing
    }
}

/// Wait for an external event or exit the SPURS thread group if no workloads can be scheduled
void spursSysServiceWaitOrExit(SPUThread & spu, SpursKernelMgmtData * mgmt) {
    // Monitor only lock line reservation lost events
    spu.WriteChannel(SPU_WrEventMask, u128::from32r(SPU_EVENT_LR));

    bool shouldExit;
    while (true) {
        spursDma(spu, MFC_GETLLAR_CMD, mgmt->spurs.addr(), 0x100/*LSA*/, 0x80/*size*/, 0/*tag*/);
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
        if (spurs->m.sysSrvMessage.read_relaxed() & (1 << mgmt->spuNum)) {
            foundReadyWorkload = true;
        } else {
            if (spurs->m.flags1 & SF1_32_WORKLOADS) {
                for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD2; i++) {
                    u32 j            = i & 0x0F;
                    u8 runnable      = i < CELL_SPURS_MAX_WORKLOAD ? mgmt->wklRunnable1 & (0x8000 >> j) : mgmt->wklRunnable2 & (0x8000 >> j);
                    u8 priority      = i < CELL_SPURS_MAX_WORKLOAD ? mgmt->priority[j] & 0x0F : mgmt->priority[j] >> 4;
                    u8 maxContention = i < CELL_SPURS_MAX_WORKLOAD ? spurs->m.wklMaxContention[j].read_relaxed() & 0x0F : spurs->m.wklMaxContention[j].read_relaxed() >> 4;
                    u8 contention    = i < CELL_SPURS_MAX_WORKLOAD ? spurs->m.wklCurrentContention[j] & 0x0F : spurs->m.wklCurrentContention[j] >> 4;
                    u8 wklSignal     = i < CELL_SPURS_MAX_WORKLOAD ? spurs->m.wklSignal1.read_relaxed() & (0x8000 >> j) : spurs->m.wklSignal2.read_relaxed() & (0x8000 >> j);
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
                    u8 runnable     = mgmt->wklRunnable1 & (0x8000 >> i);
                    u8 wklSignal    = spurs->m.wklSignal1.read_relaxed() & (0x8000 >> i);
                    u8 wklFlag      = spurs->m.wklFlag.flag.read_relaxed() == 0 ? spurs->m.wklFlagReceiver.read_relaxed() == i ? 1 : 0 : 0;
                    u8 readyCount   = spurs->m.wklReadyCount1[i].read_relaxed() > CELL_SPURS_MAX_SPU ? CELL_SPURS_MAX_SPU : spurs->m.wklReadyCount1[i].read_relaxed();
                    u8 idleSpuCount = spurs->m.wklIdleSpuCountOrReadyCount2[i].read_relaxed() > CELL_SPURS_MAX_SPU ? CELL_SPURS_MAX_SPU : spurs->m.wklIdleSpuCountOrReadyCount2[i].read_relaxed();
                    u8 requestCount = readyCount + idleSpuCount;

                    if (runnable && mgmt->priority[i] != 0 && spurs->m.wklMaxContention[i].read_relaxed() > spurs->m.wklCurrentContention[i]) {
                        if (wklFlag || wklSignal || (readyCount != 0 && requestCount > spurs->m.wklCurrentContention[i])) {
                            foundReadyWorkload = true;
                            break;
                        }
                    }
                }
            }
        }

        bool spuIdling = spurs->m.spuIdling & (1 << mgmt->spuNum) ? true : false;
        if (foundReadyWorkload && shouldExit == false) {
            spurs->m.spuIdling &= ~(1 << mgmt->spuNum);
        } else {
            spurs->m.spuIdling |= 1 << mgmt->spuNum;
        }

        // If all SPUs are idling and the exit_if_no_work flag is set then the SPU thread group must exit. Otherwise wait for external events.
        if (spuIdling && shouldExit == false && foundReadyWorkload == false) {
            // The system service blocks by making a reservation and waiting on the lock line reservation lost event.
            u128 r;
            spu.ReadChannel(r, SPU_RdEventStat);
            spu.WriteChannel(SPU_WrEventAck, u128::from32r(SPU_EVENT_LR));
        }

        auto dmaSuccess = spursDma(spu, MFC_PUTLLC_CMD, mgmt->spurs.addr(), 0x100/*LSA*/, 0x80/*size*/, 0/*tag*/);
        if (dmaSuccess && (shouldExit || foundReadyWorkload)) {
            break;
        }
    }

    if (shouldExit) {
        // TODO: exit spu thread group
    }
}

/// Main function for the system service workload
void spursSysServiceWorkloadMain(SPUThread & spu, u32 pollStatus) {
    auto mgmt = vm::get_ptr<SpursKernelMgmtData>(spu.ls_offset + 0x100);

    if (mgmt->spurs.addr() % CellSpurs::align) {
        spursHalt(spu);
        return;
    }

    // Initialise the system service if this is the first time its being started on this SPU
    if (mgmt->sysSrvInitialised == 0) {
        mgmt->sysSrvInitialised = 1;

        spursDma(spu, MFC_GETLLAR_CMD, mgmt->spurs.addr(), 0x100/*LSA*/, 0x80/*size*/, 0/*tag*/);

        do {
            spursDma(spu, MFC_GETLLAR_CMD, mgmt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/);
            CellSpurs * spurs = vm::get_ptr<CellSpurs>(spu.ls_offset + 0x2D80 - offsetof(CellSpurs, m.wklState1));

            // Halt if already initialised
            if (spurs->m.sysSrvOnSpu & (1 << mgmt->spuNum)) {
                spursHalt(spu);
                return;
            }

            spurs->m.sysSrvOnSpu |= 1 << mgmt->spuNum;
        } while (spursDma(spu, MFC_PUTLLC_CMD, mgmt->spurs.addr() + offsetof(CellSpurs, m.wklState1), 0x2D80/*LSA*/, 0x80/*size*/, 0/*tag*/) == false);

        mgmt->traceBuffer   = 0;
        mgmt->traceMsgCount = -1;
        spursSysServiceUpdateTrace(spu, mgmt, 1, 1, 0);
        spursSysServiceCleanupAfterPreemption(spu, mgmt);

        // Trace - SERVICE: INIT
        CellSpursTracePacket pkt;
        memset(&pkt, 0, sizeof(pkt));
        pkt.header.tag            = CELL_SPURS_TRACE_TAG_SERVICE;
        pkt.data.service.incident = CELL_SPURS_TRACE_SERVICE_INIT;
        cellSpursModulePutTrace(&pkt, mgmt->dmaTagId);
    }

    // Trace - START: Module='SYS '
    CellSpursTracePacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.header.tag = CELL_SPURS_TRACE_TAG_START;
    memcpy(pkt.data.start.module, "SYS ", 4);
    pkt.data.start.level = 1; // Policy module
    pkt.data.start.ls    = 0xA00 >> 2;
    cellSpursModulePutTrace(&pkt, mgmt->dmaTagId);

    while (true) {
        // Process messages for the system service workload
        spursSysServiceProcessMessages(spu, mgmt);

poll:
        if (cellSpursModulePollStatus(spu, nullptr)) {
            // Trace - SERVICE: EXIT
            CellSpursTracePacket pkt;
            memset(&pkt, 0, sizeof(pkt));
            pkt.header.tag            = CELL_SPURS_TRACE_TAG_SERVICE;
            pkt.data.service.incident = CELL_SPURS_TRACE_SERVICE_EXIT;
            cellSpursModulePutTrace(&pkt, mgmt->dmaTagId);

            // Trace - STOP: GUID
            memset(&pkt, 0, sizeof(pkt));
            pkt.header.tag = CELL_SPURS_TRACE_TAG_STOP;
            pkt.data.stop  = SPURS_GUID_SYS_WKL;
            cellSpursModulePutTrace(&pkt, mgmt->dmaTagId);

            spursDmaWaitForCompletion(spu, 1 << mgmt->dmaTagId);
            break;
        }

        // If we reach here it means that either there are more system service messages to be processed
        // or there are no workloads that can be scheduled.

        // If the SPU is not idling then process the remaining system service messages
        if (mgmt->spuIdling == 0) {
            continue;
        }

        // If we reach here it means that the SPU is idling

        // Trace - SERVICE: WAIT
        CellSpursTracePacket pkt;
        memset(&pkt, 0, sizeof(pkt));
        pkt.header.tag            = CELL_SPURS_TRACE_TAG_SERVICE;
        pkt.data.service.incident = CELL_SPURS_TRACE_SERVICE_WAIT;
        cellSpursModulePutTrace(&pkt, mgmt->dmaTagId);

        spursSysServiceWaitOrExit(spu, mgmt);
        goto poll;
    }
}

/// Entry point of the system service workload
bool spursSysServiceWorkloadEntry(SPUThread & spu) {
    auto mgmt       = vm::get_ptr<SpursKernelMgmtData>(spu.ls_offset + spu.GPR[3]._u32[3]);
    auto arg        = spu.GPR[4]._u64[1];
    auto pollStatus = spu.GPR[5]._u32[3];

    if (mgmt->wklCurrentId == CELL_SPURS_SYS_SERVICE_WORKLOAD_ID) {
        spursSysServiceWorkloadMain(spu, pollStatus);
    } else {
        // TODO: If we reach here it means the current workload was preempted to start the
        // system service workload. Need to implement this.
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////////
// SPURS taskset policy module functions
//////////////////////////////////////////////////////////////////////////////

bool spursTasksetProcessRequest(SPUThread & spu, s32 request, u32 * taskId, u32 * isWaiting) {
    auto kernelMgmt = vm::get_ptr<SpursKernelMgmtData>(spu.ls_offset + 0x100);
    auto mgmt       = vm::get_ptr<SpursTasksetPmMgmtData>(spu.ls_offset + 0x2700);

    // Verify taskset state is valid
    for (auto i = 0; i < 4; i ++) {
        if ((mgmt->taskset->m.waiting_set[i] & mgmt->taskset->m.running_set[i]) ||
            (mgmt->taskset->m.ready_set[i] & mgmt->taskset->m.ready2_set[i]) ||
            ((mgmt->taskset->m.running_set[i] | mgmt->taskset->m.ready_set[i] |
              mgmt->taskset->m.ready2_set[i] | mgmt->taskset->m.signal_received_set[i] |
              mgmt->taskset->m.waiting_set[i]) & ~mgmt->taskset->m.enabled_set[i])) {
            assert(0);
        }
    }

    // TODO: Implement cases
    s32 delta = 0;
    switch (request + 1) {
    case 0:
        break;
    case 1:
        break;
    case 2:
        break;
    case 3:
        break;
    case 4:
        break;
    case 5:
        break;
    case 6:
        break;
    default:
        assert(0);
        break;
    }

    // Set the ready count of the workload to the number of ready tasks
    do {
        s32 readyCount = kernelMgmt->wklCurrentId >= CELL_SPURS_MAX_WORKLOAD ?
                         kernelMgmt->spurs->m.wklIdleSpuCountOrReadyCount2[kernelMgmt->wklCurrentId & 0x0F].read_relaxed() :
                         kernelMgmt->spurs->m.wklReadyCount1[kernelMgmt->wklCurrentId].read_relaxed();

        auto newReadyCount = readyCount + delta > 0xFF ? 0xFF : readyCount + delta < 0 ? 0 : readyCount + delta;

        if (kernelMgmt->wklCurrentId >= CELL_SPURS_MAX_WORKLOAD) {
            kernelMgmt->spurs->m.wklIdleSpuCountOrReadyCount2[kernelMgmt->wklCurrentId & 0x0F].write_relaxed(newReadyCount);
        } else {
            kernelMgmt->spurs->m.wklReadyCount1[kernelMgmt->wklCurrentId].write_relaxed(newReadyCount);
        }

        delta += readyCount;
    } while (delta > 0);

    // TODO: Implement return
    return false;
}

void spursTasksetExit(SPUThread & spu) {
    auto mgmt = vm::get_ptr<SpursTasksetPmMgmtData>(spu.ls_offset + 0x2700);

    // Trace - STOP
    CellSpursTracePacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.header.tag = 0x54; // Its not clear what this tag means exactly but it seems similar to CELL_SPURS_TRACE_TAG_STOP
    pkt.data.stop  = SPURS_GUID_TASKSET_PM;
    cellSpursModulePutTrace(&pkt, mgmt->dmaTagId);

    // Not sure why this check exists. Perhaps to check for memory corruption.
    if (memcmp(mgmt->moduleId, "SPURSTASK MODULE", 16) != 0) {
        spursHalt(spu);
    }

    cellSpursModuleExit(spu);
}

void spursTasksetDispatch(SPUThread & spu) {
    auto mgmt       = vm::get_ptr<SpursTasksetPmMgmtData>(spu.ls_offset + 0x2700);
    auto kernelMgmt = vm::get_ptr<SpursKernelMgmtData>(spu.ls_offset + 0x100);

    u32 taskId;
    u32 isWaiting;
    spursTasksetProcessRequest(spu, 5, &taskId, &isWaiting);
    if (taskId >= CELL_SPURS_MAX_TASK) {
        spursTasksetExit(spu);
        return;
    }

    mgmt->taskId = taskId;
    u64 elfAddr  = mgmt->taskset->m.task_info[taskId].elf_addr.addr() & 0xFFFFFFFFFFFFFFF8ull;

    // Trace - Task: Incident=dispatch
    CellSpursTracePacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.header.tag = CELL_SPURS_TRACE_TAG_TASK;
    pkt.data.task.incident = CELL_SPURS_TRACE_TASK_DISPATCH;
    pkt.data.task.taskId   = taskId;
    cellSpursModulePutTrace(&pkt, 0x1F);

    if (isWaiting == 0) {
    }

    if (mgmt->taskset->m.enable_clear_ls) {
        memset(vm::get_ptr<void>(spu.ls_offset + CELL_SPURS_TASK_TOP), 0, CELL_SPURS_TASK_BOTTOM - CELL_SPURS_TASK_TOP);
    }

    // If the entire LS is saved then there is no need to load the ELF as it will be be saved in the context save area
    if (mgmt->taskset->m.task_info[taskId].ls_pattern.u64[0] != 0xFFFFFFFFFFFFFFFFull ||
        (mgmt->taskset->m.task_info[taskId].ls_pattern.u64[0] | 0xFC00000000000000ull) != 0xFFFFFFFFFFFFFFFFull) {
        // Load the ELF
        // TODO: Load ELF
    }

    // Load save context from main memory to LS
    u64 context_save_storage = mgmt->taskset->m.task_info[taskId].context_save_storage_and_alloc_ls_blocks & 0xFFFFFFFFFFFFFF80ull;
    for (auto i = 6; i < 128; i++) {
        bool shouldLoad = mgmt->taskset->m.task_info[taskId].ls_pattern.u64[i < 64 ? 1 : 0] & (0x8000000000000000ull >> i) ? true : false;
        if (shouldLoad) {
            memcpy(vm::get_ptr<void>(spu.ls_offset + CELL_SPURS_TASK_TOP + ((i - 6) << 11)),
                   vm::get_ptr<void>((u32)context_save_storage + 0x400 + ((i - 6) << 11)), 0x800);
        }
    }

    // Trace - GUID
    memset(&pkt, 0, sizeof(pkt));
    pkt.header.tag = CELL_SPURS_TRACE_TAG_GUID;
    pkt.data.guid  = 0; // TODO: Put GUID of taskId here
    cellSpursModulePutTrace(&pkt, 0x1F);
}

void spursTasksetProcessPollStatus(SPUThread & spu, u32 pollStatus) {
    if (pollStatus & CELL_SPURS_MODULE_POLL_STATUS_FLAG) {
        spursTasksetProcessRequest(spu, 6, nullptr, nullptr);
    }
}

bool spursTasksetShouldYield(SPUThread & spu) {
    u32 pollStatus;

    if (cellSpursModulePollStatus(spu, &pollStatus)) {
        return true;
    }

    spursTasksetProcessPollStatus(spu, pollStatus);
    return false;
}

void spursTasksetInit(SPUThread & spu, u32 pollStatus) {
    auto mgmt       = vm::get_ptr<SpursTasksetPmMgmtData>(spu.ls_offset + 0x2700);
    auto kernelMgmt = vm::get_ptr<SpursKernelMgmtData>(spu.ls_offset + 0x100);

    kernelMgmt->moduleId[0] = 'T';
    kernelMgmt->moduleId[1] = 'K';

    // Trace - START: Module='TKST'
    CellSpursTracePacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.header.tag = 0x52; // Its not clear what this tag means exactly but it seems similar to CELL_SPURS_TRACE_TAG_START
    memcpy(pkt.data.start.module, "TKST", 4);
    pkt.data.start.level = 2;
    pkt.data.start.ls    = 0xA00 >> 2;
    cellSpursModulePutTrace(&pkt, mgmt->dmaTagId);

    spursTasksetProcessPollStatus(spu, pollStatus);
}

void spursTasksetResumeTask(SPUThread & spu) {
    auto mgmt = vm::get_ptr<SpursTasksetPmMgmtData>(spu.ls_offset + 0x2700);

    // Restore task context
    spu.GPR[0] = mgmt->savedContextLr;
    spu.GPR[1] = mgmt->savedContextSp;
    for (auto i = 0; i < 48; i++) {
        spu.GPR[80 + i] = mgmt->savedContextR80ToR127[i];
    }

    spu.SetBranch(spu.GPR[0]._u32[3]);
}

bool spursTasksetEntry(SPUThread & spu) {
    auto mgmt = vm::get_ptr<SpursTasksetPmMgmtData>(spu.ls_offset + 0x2700);

    if (spu.PC == CELL_SPURS_TASKSET_PM_ENTRY_ADDR) {
        // Called from kernel
        auto kernelMgmt = vm::get_ptr<SpursKernelMgmtData>(spu.ls_offset + spu.GPR[3]._u32[3]);
        auto arg        = spu.GPR[4]._u64[1];
        auto pollStatus = spu.GPR[5]._u32[3];

        memset(mgmt, 0, sizeof(*mgmt));
        mgmt->taskset.set(arg);
        memcpy(mgmt->moduleId, "SPURSTASK MODULE", 16);
        mgmt->kernelMgmtAddr = spu.GPR[3]._u32[3];
        mgmt->syscallAddr    = CELL_SPURS_TASKSET_PM_SYSCALL_ADDR;
        mgmt->spuNum         = kernelMgmt->spuNum;
        mgmt->dmaTagId       = kernelMgmt->dmaTagId;
        mgmt->taskId         = 0xFFFFFFFF;

        // Register SPURS takset policy module HLE functions
        spu.UnregisterHleFunctions(CELL_SPURS_TASKSET_PM_ENTRY_ADDR, 0x40000); // TODO: use a symbolic constant
        spu.RegisterHleFunction(CELL_SPURS_TASKSET_PM_ENTRY_ADDR, spursTasksetEntry);
        spu.RegisterHleFunction(mgmt->syscallAddr, spursTasksetEntry);

        spursTasksetInit(spu, pollStatus);
        spursTasksetDispatch(spu);
    } else if (spu.PC == CELL_SPURS_TASKSET_PM_SYSCALL_ADDR) {
        // Save task context
        mgmt->savedContextLr = spu.GPR[0];
        mgmt->savedContextSp = spu.GPR[1];
        for (auto i = 0; i < 48; i++) {
            mgmt->savedContextR80ToR127[i] = spu.GPR[80 + i];
        }

        // TODO: Process syscall
        spursTasksetResumeTask(spu);
    } else {
        assert(0);
    }

    return false;
}
