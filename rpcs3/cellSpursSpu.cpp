#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/SysCalls/Modules/cellSpurs.h"

/// Output trace information
void cellSpursModulePutTrace(CellSpursTracePacket * packet, unsigned tag) {
    // TODO: Implement this
}

/// Check for execution right requests
unsigned cellSpursModulePollStatus(CellSpursModulePollStatus * status) {
    // TODO: Implement this
    return 0;
}

void spursSysServiceCleanup(SPUThread & spu, SpursKernelMgmtData * mgmt) {
    // TODO: Implement this
}

void spursSysServiceUpdateTrace(SPUThread & spu, SpursKernelMgmtData * mgmt, u32 arg2, u32 arg3, u32 arg4) {
    // TODO: Implement this
}

void spursSysServiceUpdateEvent(SPUThread & spu, SpursKernelMgmtData * mgmt, u32 wklShutdownMask) {
    u32 wklNotifyMask = 0;
    for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
        if (wklShutdownMask & (0x80000000 >> i)) {
            mgmt->spurs->m.wklEvent1[i] |= 0x01;
            if (mgmt->spurs->m.wklEvent1[i] & 0x02 || mgmt->spurs->m.wklEvent1[i] & 0x10) {
                wklNotifyMask |= 0x80000000 >> i;
            }
        }
    }

    for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
        if (wklShutdownMask & (0x8000 >> i)) {
            mgmt->spurs->m.wklEvent2[i] |= 0x01;
            if (mgmt->spurs->m.wklEvent2[i] & 0x02 || mgmt->spurs->m.wklEvent2[i] & 0x10) {
                wklNotifyMask |= 0x8000 >> i;
            }
        }
    }

    if (wklNotifyMask) {
        // TODO: sys_spu_thread_send_event(mgmt->spurs->m.spuPort, 0, wklNotifyMask);
    }
}

/// Update workload information in the LS from main memory
void spursSysServiceUpdateWorkload(SPUThread & spu, SpursKernelMgmtData * mgmt) {
    u32 wklShutdownMask = 0;
    mgmt->wklRunnable1  = 0;
    for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
        mgmt->priority[i]    = mgmt->spurs->m.wklInfo1[i].priority[mgmt->spuNum] == 0 ? 0 : 0x10 - mgmt->spurs->m.wklInfo1[i].priority[mgmt->spuNum];
        mgmt->wklUniqueId[i] = mgmt->spurs->m.wklInfo1[i].uniqueId.read_relaxed();

        auto wklStatus = mgmt->spurs->m.wklStatus1[i];
        if (mgmt->spurs->m.wklState1[i].read_relaxed() == SPURS_WKL_STATE_RUNNABLE) {
            mgmt->spurs->m.wklStatus1[i] |= 1 << mgmt->spuNum;
            mgmt->wklRunnable1           |= 0x8000 >> i;
        } else {
            mgmt->spurs->m.wklStatus1[i] &= ~(1 << mgmt->spuNum);
        }

        if (mgmt->spurs->m.wklState1[i].read_relaxed() == SPURS_WKL_STATE_SHUTTING_DOWN) {
            if (((wklStatus & (1 << mgmt->spuNum)) != 0) && ((mgmt->spurs->m.wklStatus1[i] & (1 << mgmt->spuNum)) == 0)) {
                mgmt->spurs->m.wklState1[i].write_relaxed(SPURS_WKL_STATE_REMOVABLE);
                wklShutdownMask |= 0x80000000 >> i;
            }
        }
    }

    if (mgmt->spurs->m.flags1 & SF1_32_WORKLOADS) {
        mgmt->wklRunnable2 = 0;
        for (u32 i = 0; i < CELL_SPURS_MAX_WORKLOAD; i++) {
            if (mgmt->spurs->m.wklInfo2[i].priority[mgmt->spuNum]) {
                mgmt->priority[i] |= (0x10 - mgmt->spurs->m.wklInfo2[i].priority[mgmt->spuNum]) << 4;
            }

            auto wklStatus = mgmt->spurs->m.wklStatus2[i];
            if (mgmt->spurs->m.wklState2[i].read_relaxed() == SPURS_WKL_STATE_RUNNABLE) {
                mgmt->spurs->m.wklStatus2[i] |= 1 << mgmt->spuNum;
                mgmt->wklRunnable2           |= 0x8000 >> i;
            } else {
                mgmt->spurs->m.wklStatus2[i] &= ~(1 << mgmt->spuNum);
            }

            if (mgmt->spurs->m.wklState2[i].read_relaxed() == SPURS_WKL_STATE_SHUTTING_DOWN) {
                if (((wklStatus & (1 << mgmt->spuNum)) != 0) && ((mgmt->spurs->m.wklStatus2[i] & (1 << mgmt->spuNum)) == 0)) {
                    mgmt->spurs->m.wklState2[i].write_relaxed(SPURS_WKL_STATE_REMOVABLE);
                    wklShutdownMask |= 0x8000 >> i;
                }
            }
        }
    }

    if (wklShutdownMask) {
        spursSysServiceUpdateEvent(spu, mgmt, wklShutdownMask);
    }
}

/// Process any messages
void spursSysServiceProcessMessages(SPUThread & spu, SpursKernelMgmtData * mgmt) {
    if (mgmt->spurs->m.sysSrvMsgUpdateWorkload.read_relaxed() & (1 << mgmt->spuNum)) {
        mgmt->spurs->m.sysSrvMsgUpdateWorkload &= ~(1 << mgmt->spuNum);
        spursSysServiceUpdateWorkload(spu, mgmt);
    }

    if (mgmt->spurs->m.sysSrvMsgUpdateTrace & (1 << mgmt->spuNum)) {
        spursSysServiceUpdateTrace(spu, mgmt, 1, 0, 0);
    }

    if (mgmt->spurs->m.sysSrvMsgTerminate & (1 << mgmt->spuNum)) {
        mgmt->spurs->m.sysSrvOnSpu &= ~(1 << mgmt->spuNum);
        // TODO: Rest of the terminate processing
    }
}

void spursSysServiceExitIfRequired() {
    // TODO: Implement this
}

/// Main function for the system service workload
void spursSysServiceWorkloadMain(SPUThread & spu, u32 pollStatus) {
    auto mgmt = vm::get_ptr<SpursKernelMgmtData>(spu.ls_offset);

    if (mgmt->spurs.addr() % CellSpurs::align) {
        // TODO: Halt
    }

    // Initialise the system service if this is the first time its being started on this SPU
    if (mgmt->sysSrvInitialised != 0) {
        mgmt->sysSrvInitialised = 1;

        if (mgmt->spurs->m.sysSrvOnSpu & (1 << mgmt->spuNum)) {
            // TODO: Halt
        }

        mgmt->spurs->m.sysSrvOnSpu |= 1 << mgmt->spuNum;
        mgmt->traceBuffer           = 0;
        mgmt->traceMsgCount         = -1;

        spursSysServiceUpdateTrace(spu, mgmt, 1, 1, 0);
        spursSysServiceCleanup(spu, mgmt);

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
        spursSysServiceProcessMessages(spu, mgmt);

        if (cellSpursModulePollStatus(nullptr)) {
            // Trace - SERVICE: EXIT
            CellSpursTracePacket pkt;
            memset(&pkt, 0, sizeof(pkt));
            pkt.header.tag            = CELL_SPURS_TRACE_TAG_SERVICE;
            pkt.data.service.incident = CELL_SPURS_TRACE_SERVICE_EXIT;
            cellSpursModulePutTrace(&pkt, mgmt->dmaTagId);

            // Trace - STOP: GUID
            memset(&pkt, 0, sizeof(pkt));
            pkt.header.tag = CELL_SPURS_TRACE_TAG_STOP;
            pkt.data.stop  = 0; // TODO: Put GUID of the sys service workload here
            cellSpursModulePutTrace(&pkt, mgmt->dmaTagId);
            break;
        }

        if (mgmt->spuIdling == 0) {
            continue;
        }

        // Trace - SERVICE: WAIT
        CellSpursTracePacket pkt;
        memset(&pkt, 0, sizeof(pkt));
        pkt.header.tag            = CELL_SPURS_TRACE_TAG_SERVICE;
        pkt.data.service.incident = CELL_SPURS_TRACE_SERVICE_WAIT;
        cellSpursModulePutTrace(&pkt, mgmt->dmaTagId);

        spursSysServiceExitIfRequired();
    }
}

/// Entry point of the system service workload
void spursSysServiceWorkloadEntry(SPUThread & spu) {
    auto mgmt       = vm::get_ptr<SpursKernelMgmtData>(spu.ls_offset + spu.GPR[3]._u32[3]);
    auto arg        = spu.GPR[4]._u64[1];
    auto pollStatus = spu.GPR[5]._u32[3];

    spu.GPR[1]._u32[3]                        = 0x3FFD0;
    *(vm::ptr<u32>::make(spu.GPR[1]._u32[3])) = 0x3FFF0;
    memset(vm::get_ptr<void>(spu.ls_offset + 0x3FFE0), 0, 32);

    LV2_LOCK(0);

    if (mgmt->wklCurrentId == CELL_SPURS_SYS_SERVICE_WORKLOAD_ID) {
        spursSysServiceWorkloadMain(spu, pollStatus);
    } else {
        // TODO: If we reach here it means the current workload was preempted to start the
        // system service workload. Need to implement this.
    }

    // TODO: Ensure that this function always returns to the SPURS kernel
    return;
}
