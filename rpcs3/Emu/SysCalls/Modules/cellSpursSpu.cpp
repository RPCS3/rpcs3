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

bool spursDma(SPUThread & spu, u32 cmd, u64 ea, u32 lsa, u32 size, u32 tag);
u32 spursDmaGetCompletionStatus(SPUThread & spu, u32 tagMask);
u32 spursDmaWaitForCompletion(SPUThread & spu, u32 tagMask, bool waitForAll = true);

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
// SPURS taskset polict module functions
//

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
        CellSpurs * spurs = (CellSpurs *)mgmt->tempArea;

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
        CellSpurs * spurs = (CellSpurs *)mgmt->tempArea;

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
    u32 pollStatus;
    const CellSpurs::WorkloadInfo * wklInfo;
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

        spu.UnregisterHleFunctions(0, 0x40000); // TODO: use a symbolic constant
        spu.RegisterHleFuncion(isKernel2 ? CELL_SPURS_KERNEL2_ENTRY_ADDR : CELL_SPURS_KERNEL1_ENTRY_ADDR, spursKernelMain);
        spu.RegisterHleFuncion(mgmt->yieldToKernelAddr, spursKernelMain);
        spu.RegisterHleFuncion(mgmt->selectWorkloadAddr, isKernel2 ? spursKernel2SelectWorkload : spursKernel1SelectWorkload);

        // Start the system service workload
        spu.RegisterHleFuncion(0xA00, spursSysServiceWorkloadEntry);
        wklInfo    = &mgmt->spurs->m.wklInfoSysSrv;
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
        wklInfo    = wid < CELL_SPURS_MAX_WORKLOAD ? &mgmt->spurs->m.wklInfo1[wid] :
                        (wid < CELL_SPURS_MAX_WORKLOAD2 && isKernel2 ? &mgmt->spurs->m.wklInfo2[wid & 0xf] : &mgmt->spurs->m.wklInfoSysSrv);
    } else {
        assert(0);
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
    if (mgmt->spurs->m.sysSrvWorkload[mgmt->spuNum] != 0xFF) {
        auto wklId = mgmt->spurs->m.sysSrvWorkload[mgmt->spuNum];
        mgmt->spurs->m.sysSrvWorkload[mgmt->spuNum] = 0xFF;

        spursSysServiceUpdateWorkload(spu, mgmt);
        if (wklId >= CELL_SPURS_MAX_WORKLOAD) {
            mgmt->spurs->m.wklCurrentContention[wklId & 0x0F] -= 0x10;
            mgmt->spurs->m.wklReadyCount1[wklId & 0x0F].write_relaxed(mgmt->spurs->m.wklReadyCount1[wklId & 0x0F].read_relaxed() - 1);
        } else {
            mgmt->spurs->m.wklCurrentContention[wklId & 0x0F] -= 0x01;
            mgmt->spurs->m.wklIdleSpuCountOrReadyCount2[wklId & 0x0F].write_relaxed(mgmt->spurs->m.wklIdleSpuCountOrReadyCount2[wklId & 0x0F].read_relaxed() - 1);
        }

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
    auto sysSrvMsgUpdateTrace           = mgmt->spurs->m.sysSrvMsgUpdateTrace;
    mgmt->spurs->m.sysSrvMsgUpdateTrace &= ~(1 << mgmt->spuNum);
    mgmt->spurs->m.xCC                  &= ~(1 << mgmt->spuNum);
    mgmt->spurs->m.xCC                  |= arg2 << mgmt->spuNum;

    bool notify = false;
    if (((sysSrvMsgUpdateTrace & (1 << mgmt->spuNum)) != 0) && (mgmt->spurs->m.sysSrvMsgUpdateTrace == 0) && (mgmt->spurs->m.xCD != 0)) {
        mgmt->spurs->m.xCD = 0;
        notify             = true;
    }

    if (arg4 && mgmt->spurs->m.xCD != 0) {
        mgmt->spurs->m.xCD = 0;
        notify             = true;
    }

    // Get trace parameters from CellSpurs and store them in the LS
    if (((sysSrvMsgUpdateTrace & (1 << mgmt->spuNum)) != 0) || (arg3 != 0)) {
        if (mgmt->traceMsgCount != 0xFF || mgmt->spurs->m.traceBuffer.addr() == 0) {
            spursSysServiceUpdateTraceCount(spu, mgmt);
        } else {
            mgmt->traceMsgCount = mgmt->spurs->m.traceBuffer->count[mgmt->spuNum];
        }

        mgmt->traceBuffer   = mgmt->spurs->m.traceBuffer.addr() + (mgmt->spurs->m.traceStartIndex[mgmt->spuNum] << 4);
        mgmt->traceMaxCount = mgmt->spurs->m.traceStartIndex[1] - mgmt->spurs->m.traceStartIndex[0];
        if (mgmt->traceBuffer == 0) {
            mgmt->traceMsgCount = 0;
        }
    }

    if (notify) {
        // TODO: sys_spu_thread_send_event(mgmt->spurs->m.spuPort, 2, 0);
    }
}

/// Update events in CellSpurs
void spursSysServiceUpdateEvent(SPUThread & spu, SpursKernelMgmtData * mgmt, u32 wklShutdownBitSet) {
    // Mark the workloads in wklShutdownBitSet as completed and also generate a bit set of the completed
    // workloads that have a shutdown completion hook registered
    u32 wklNotifyBitSet = 0;
    for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
        if (wklShutdownBitSet & (0x80000000u >> i)) {
            mgmt->spurs->m.wklEvent1[i] |= 0x01;
            if (mgmt->spurs->m.wklEvent1[i] & 0x02 || mgmt->spurs->m.wklEvent1[i] & 0x10) {
                wklNotifyBitSet |= 0x80000000u >> i;
            }
        }

        if (wklShutdownBitSet & (0x8000 >> i)) {
            mgmt->spurs->m.wklEvent2[i] |= 0x01;
            if (mgmt->spurs->m.wklEvent2[i] & 0x02 || mgmt->spurs->m.wklEvent2[i] & 0x10) {
                wklNotifyBitSet |= 0x8000 >> i;
            }
        }
    }

    if (wklNotifyBitSet) {
        // TODO: sys_spu_thread_send_event(mgmt->spurs->m.spuPort, 0, wklNotifyMask);
    }
}

/// Update workload information in the SPU from CellSpurs
void spursSysServiceUpdateWorkload(SPUThread & spu, SpursKernelMgmtData * mgmt) {
    u32 wklShutdownBitSet = 0;
    mgmt->wklRunnable1    = 0;
    mgmt->wklRunnable2    = 0;
    for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
        // Copy the priority of the workload for this SPU and its unique id to the LS
        mgmt->priority[i]    = mgmt->spurs->m.wklInfo1[i].priority[mgmt->spuNum] == 0 ? 0 : 0x10 - mgmt->spurs->m.wklInfo1[i].priority[mgmt->spuNum];
        mgmt->wklUniqueId[i] = mgmt->spurs->m.wklInfo1[i].uniqueId.read_relaxed();

        // Update workload status and runnable flag based on the workload state
        auto wklStatus = mgmt->spurs->m.wklStatus1[i];
        if (mgmt->spurs->m.wklState1[i].read_relaxed() == SPURS_WKL_STATE_RUNNABLE) {
            mgmt->spurs->m.wklStatus1[i] |= 1 << mgmt->spuNum;
            mgmt->wklRunnable1           |= 0x8000 >> i;
        } else {
            mgmt->spurs->m.wklStatus1[i] &= ~(1 << mgmt->spuNum);
        }

        // If the workload is shutting down and if this is the last SPU from which it is being removed then
        // add it to the shutdown bit set
        if (mgmt->spurs->m.wklState1[i].read_relaxed() == SPURS_WKL_STATE_SHUTTING_DOWN) {
            if (((wklStatus & (1 << mgmt->spuNum)) != 0) && (mgmt->spurs->m.wklStatus1[i] == 0)) {
                mgmt->spurs->m.wklState1[i].write_relaxed(SPURS_WKL_STATE_REMOVABLE);
                wklShutdownBitSet |= 0x80000000u >> i;
            }
        }

        if (mgmt->spurs->m.flags1 & SF1_32_WORKLOADS) {
            // Copy the priority of the workload for this SPU to the LS
            if (mgmt->spurs->m.wklInfo2[i].priority[mgmt->spuNum]) {
                mgmt->priority[i] |= (0x10 - mgmt->spurs->m.wklInfo2[i].priority[mgmt->spuNum]) << 4;
            }

            // Update workload status and runnable flag based on the workload state
            wklStatus = mgmt->spurs->m.wklStatus2[i];
            if (mgmt->spurs->m.wklState2[i].read_relaxed() == SPURS_WKL_STATE_RUNNABLE) {
                mgmt->spurs->m.wklStatus2[i] |= 1 << mgmt->spuNum;
                mgmt->wklRunnable2           |= 0x8000 >> i;
            } else {
                mgmt->spurs->m.wklStatus2[i] &= ~(1 << mgmt->spuNum);
            }

            // If the workload is shutting down and if this is the last SPU from which it is being removed then
            // add it to the shutdown bit set
            if (mgmt->spurs->m.wklState2[i].read_relaxed() == SPURS_WKL_STATE_SHUTTING_DOWN) {
                if (((wklStatus & (1 << mgmt->spuNum)) != 0) && (mgmt->spurs->m.wklStatus2[i] == 0)) {
                    mgmt->spurs->m.wklState2[i].write_relaxed(SPURS_WKL_STATE_REMOVABLE);
                    wklShutdownBitSet |= 0x8000 >> i;
                }
            }
        }
    }

    if (wklShutdownBitSet) {
        spursSysServiceUpdateEvent(spu, mgmt, wklShutdownBitSet);
    }
}

/// Process any messages
void spursSysServiceProcessMessages(SPUThread & spu, SpursKernelMgmtData * mgmt) {
    LV2_LOCK(0);

    // Process update workload message
    if (mgmt->spurs->m.sysSrvMsgUpdateWorkload.read_relaxed() & (1 << mgmt->spuNum)) {
        mgmt->spurs->m.sysSrvMsgUpdateWorkload &= ~(1 << mgmt->spuNum);
        spursSysServiceUpdateWorkload(spu, mgmt);
    }

    // Process update trace message
    if (mgmt->spurs->m.sysSrvMsgUpdateTrace & (1 << mgmt->spuNum)) {
        spursSysServiceUpdateTrace(spu, mgmt, 1, 0, 0);
    }

    // Process terminate request
    if (mgmt->spurs->m.sysSrvMsgTerminate & (1 << mgmt->spuNum)) {
        mgmt->spurs->m.sysSrvOnSpu &= ~(1 << mgmt->spuNum);
        // TODO: Rest of the terminate processing
    }
}

/// Wait for an external event or exit the SPURS thread group if no workloads can be scheduled
void spursSysServiceWaitOrExit(SPUThread & spu, SpursKernelMgmtData * mgmt) {
    while (true) {
        Emu.GetCoreMutex().lock();

        // Find the number of SPUs that are idling in this SPURS instance
        u32 nIdlingSpus = 0;
        for (u32 i = 0; i < 8; i++) {
            if (mgmt->spurs->m.spuIdling & (1 << i)) {
                nIdlingSpus++;
            }
        }

        bool allSpusIdle  = nIdlingSpus == mgmt->spurs->m.nSpus ? true: false;
        bool exitIfNoWork = mgmt->spurs->m.flags1 & SF1_EXIT_IF_NO_WORK ? true : false;

        // Check if any workloads can be scheduled
        bool foundReadyWorkload = false;
        if (mgmt->spurs->m.sysSrvMessage.read_relaxed() & (1 << mgmt->spuNum)) {
            foundReadyWorkload = true;
        } else {
            if (mgmt->spurs->m.flags1 & SF1_32_WORKLOADS) {
                for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD2; i++) {
                    u32 j            = i & 0x0F;
                    u8 runnable      = i < CELL_SPURS_MAX_WORKLOAD ? mgmt->wklRunnable1 & (0x8000 >> j) : mgmt->wklRunnable2 & (0x8000 >> j);
                    u8 priority      = i < CELL_SPURS_MAX_WORKLOAD ? mgmt->priority[j] & 0x0F : mgmt->priority[j] >> 4;
                    u8 maxContention = i < CELL_SPURS_MAX_WORKLOAD ? mgmt->spurs->m.wklMaxContention[j].read_relaxed() & 0x0F : mgmt->spurs->m.wklMaxContention[j].read_relaxed() >> 4;
                    u8 contention    = i < CELL_SPURS_MAX_WORKLOAD ? mgmt->spurs->m.wklCurrentContention[j] & 0x0F : mgmt->spurs->m.wklCurrentContention[j] >> 4;
                    u8 wklSignal     = i < CELL_SPURS_MAX_WORKLOAD ? mgmt->spurs->m.wklSignal1.read_relaxed() & (0x8000 >> j) : mgmt->spurs->m.wklSignal2.read_relaxed() & (0x8000 >> j);
                    u8 wklFlag       = mgmt->spurs->m.wklFlag.flag.read_relaxed() == 0 ? mgmt->spurs->m.wklFlagReceiver.read_relaxed() == i ? 1 : 0 : 0;
                    u8 readyCount    = i < CELL_SPURS_MAX_WORKLOAD ? mgmt->spurs->m.wklReadyCount1[j].read_relaxed() : mgmt->spurs->m.wklIdleSpuCountOrReadyCount2[j].read_relaxed();

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
                    u8 wklSignal    = mgmt->spurs->m.wklSignal1.read_relaxed() & (0x8000 >> i);
                    u8 wklFlag      = mgmt->spurs->m.wklFlag.flag.read_relaxed() == 0 ? mgmt->spurs->m.wklFlagReceiver.read_relaxed() == i ? 1 : 0 : 0;
                    u8 readyCount   = mgmt->spurs->m.wklReadyCount1[i].read_relaxed() > CELL_SPURS_MAX_SPU ? CELL_SPURS_MAX_SPU : mgmt->spurs->m.wklReadyCount1[i].read_relaxed();
                    u8 idleSpuCount = mgmt->spurs->m.wklIdleSpuCountOrReadyCount2[i].read_relaxed() > CELL_SPURS_MAX_SPU ? CELL_SPURS_MAX_SPU : mgmt->spurs->m.wklIdleSpuCountOrReadyCount2[i].read_relaxed();
                    u8 requestCount = readyCount + idleSpuCount;

                    if (runnable && mgmt->priority[i] != 0 && mgmt->spurs->m.wklMaxContention[i].read_relaxed() > mgmt->spurs->m.wklCurrentContention[i]) {
                        if (wklFlag || wklSignal || (readyCount != 0 && requestCount > mgmt->spurs->m.wklCurrentContention[i])) {
                            foundReadyWorkload = true;
                            break;
                        }
                    }
                }
            }
        }

        // If all SPUs are idling and the exit_if_no_work flag is set then the SPU thread group must exit. Otherwise wait for external events.
        if ((mgmt->spurs->m.spuIdling & (1 << mgmt->spuNum)) && (allSpusIdle == false || exitIfNoWork == false) && foundReadyWorkload == false) {
            // The system service blocks by making a reservation and waiting on the reservation lost event. This is unfortunately
            // not yet completely implemented in rpcs3. So we busy wait here.
            //u128 r;
            //spu.ReadChannel(r, 0);

            Emu.GetCoreMutex().unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            Emu.GetCoreMutex().lock();
        }

        if ((allSpusIdle == true && exitIfNoWork == true) || foundReadyWorkload == false) {
            mgmt->spurs->m.spuIdling |= 1 << mgmt->spuNum;
        } else {
            mgmt->spurs->m.spuIdling &= ~(1 << mgmt->spuNum);
        }

        Emu.GetCoreMutex().unlock();

        if (allSpusIdle == false || exitIfNoWork == false) {
            if (foundReadyWorkload == true) {
                return;
            }
        } else {
            // TODO: exit spu thread group
        }
    }
}

/// Main function for the system service workload
void spursSysServiceWorkloadMain(SPUThread & spu, u32 pollStatus) {
    auto mgmt = vm::get_ptr<SpursKernelMgmtData>(spu.ls_offset + 0x100);

    if (mgmt->spurs.addr() % CellSpurs::align) {
        assert(0);
    }

    // Initialise the system service if this is the first time its being started on this SPU
    if (mgmt->sysSrvInitialised == 0) {
        mgmt->sysSrvInitialised = 1;

        LV2_LOCK(0);
        if (mgmt->spurs->m.sysSrvOnSpu & (1 << mgmt->spuNum)) {
            assert(0);
        }

        mgmt->spurs->m.sysSrvOnSpu |= 1 << mgmt->spuNum;
        mgmt->traceBuffer           = 0;
        mgmt->traceMsgCount         = -1;

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

    spu.GPR[1]._u32[3]                        = 0x3FFD0;
    *(vm::ptr<u32>::make(spu.GPR[1]._u32[3])) = 0x3FFF0;
    memset(vm::get_ptr<void>(spu.ls_offset + 0x3FFE0), 0, 32);

    if (mgmt->wklCurrentId == CELL_SPURS_SYS_SERVICE_WORKLOAD_ID) {
        spursSysServiceWorkloadMain(spu, pollStatus);
    } else {
        // TODO: If we reach here it means the current workload was preempted to start the
        // system service workload. Need to implement this.
    }

    // TODO: Ensure that this function always returns to the SPURS kernel
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

void spursTasksetDispatch(SPUThread & spu) {
    auto mgmt       = vm::get_ptr<SpursTasksetPmMgmtData>(spu.ls_offset + 0x2700);
    auto kernelMgmt = vm::get_ptr<SpursKernelMgmtData>(spu.ls_offset + 0x100);

    u32 taskId;
    u32 isWaiting;
    spursTasksetProcessRequest(spu, 5, &taskId, &isWaiting);
    if (taskId >= CELL_SPURS_MAX_TASK) {
        // TODO: spursTasksetExit(spu);
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

void spursTasksetEntry(SPUThread & spu) {
    auto mgmt = vm::get_ptr<SpursTasksetPmMgmtData>(spu.ls_offset + 0x2700);

    // Check if the function was invoked by the SPURS kernel or because of a syscall
    if (spu.PC != 0xA70) {
        // Called from kernel
        auto kernelMgmt = vm::get_ptr<SpursKernelMgmtData>(spu.ls_offset + spu.GPR[3]._u32[3]);
        auto arg        = spu.GPR[4]._u64[1];
        auto pollStatus = spu.GPR[5]._u32[3];

        memset(mgmt, 0, sizeof(*mgmt));
        mgmt->taskset.set(arg);
        memcpy(mgmt->moduleId, "SPURSTASK MODULE", 16);
        mgmt->kernelMgmtAddr = spu.GPR[3]._u32[3];
        mgmt->yieldAddr      = 0xA70;
        mgmt->spuNum         = kernelMgmt->spuNum;
        mgmt->dmaTagId       = kernelMgmt->dmaTagId;
        mgmt->taskId         = 0xFFFFFFFF;

        spursTasksetInit(spu, pollStatus);
        // TODO: Dispatch
    }

    mgmt->savedContextLr = spu.GPR[0];
    mgmt->savedContextSp = spu.GPR[1];
    for (auto i = 0; i < 48; i++) {
        mgmt->savedContextR80ToR127[i] = spu.GPR[80 + i];
    }

    // TODO: Process syscall
}
