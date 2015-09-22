#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "cellFiber.h"

extern Module<> cellFiber;

s32 _cellFiberPpuInitialize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 _cellFiberPpuSchedulerAttributeInitialize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuInitializeScheduler()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuFinalizeScheduler()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuRunFibers()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuCheckFlags()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuHasRunnableFiber()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 _cellFiberPpuAttributeInitialize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuCreateFiber()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuExit()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuYield()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuJoinFiber()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

vm::ptr<void> cellFiberPpuSelf()
{
	cellFiber.Log("cellFiberPpuSelf() -> nullptr"); // TODO

	// returns fiber structure (zero for simple PPU thread)
	return vm::null;
}

s32 cellFiberPpuSendSignal()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuWaitSignal()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuWaitFlag()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuGetScheduler()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuSetPriority()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuCheckStackLimit()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 _cellFiberPpuContextAttributeInitialize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuContextInitialize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuContextFinalize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuContextRun()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuContextSwitch()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuContextSelf()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuContextReturnToThread()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuContextCheckStackLimit()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuContextRunScheduler()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuContextEnterScheduler()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuSchedulerTraceInitialize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuSchedulerTraceFinalize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuSchedulerTraceStart()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuSchedulerTraceStop()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 _cellFiberPpuUtilWorkerControlAttributeInitialize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuUtilWorkerControlRunFibers()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuUtilWorkerControlInitialize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuUtilWorkerControlSetPollingMode()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuUtilWorkerControlJoinFiber()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuUtilWorkerControlDisconnectEventQueue()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuUtilWorkerControlSendSignal()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuUtilWorkerControlConnectEventQueueToSpurs()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuUtilWorkerControlFinalize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuUtilWorkerControlWakeup()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuUtilWorkerControlCreateFiber()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuUtilWorkerControlShutdown()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuUtilWorkerControlCheckFlags()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

s32 cellFiberPpuUtilWorkerControlInitializeWithAttribute()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

Module<> cellFiber("cellFiber", []()
{
	REG_FUNC(cellFiber, _cellFiberPpuInitialize, MFF_NO_RETURN);

	REG_FUNC(cellFiber, _cellFiberPpuSchedulerAttributeInitialize, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuInitializeScheduler, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuFinalizeScheduler, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuRunFibers, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuCheckFlags, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuHasRunnableFiber, MFF_NO_RETURN);

	REG_FUNC(cellFiber, _cellFiberPpuAttributeInitialize, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuCreateFiber, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuExit, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuYield, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuJoinFiber, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuSelf);
	REG_FUNC(cellFiber, cellFiberPpuSendSignal, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuWaitSignal, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuWaitFlag, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuGetScheduler, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuSetPriority, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuCheckStackLimit, MFF_NO_RETURN);

	REG_FUNC(cellFiber, _cellFiberPpuContextAttributeInitialize, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuContextInitialize, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuContextFinalize, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuContextRun, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuContextSwitch, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuContextSelf, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuContextReturnToThread, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuContextCheckStackLimit, MFF_NO_RETURN);

	REG_FUNC(cellFiber, cellFiberPpuContextRunScheduler, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuContextEnterScheduler, MFF_NO_RETURN);

	REG_FUNC(cellFiber, cellFiberPpuSchedulerTraceInitialize, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuSchedulerTraceFinalize, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuSchedulerTraceStart, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuSchedulerTraceStop, MFF_NO_RETURN);

	REG_FUNC(cellFiber, _cellFiberPpuUtilWorkerControlAttributeInitialize, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlRunFibers, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlInitialize, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlSetPollingMode, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlJoinFiber, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlDisconnectEventQueue, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlSendSignal, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlConnectEventQueueToSpurs, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlFinalize, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlWakeup, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlCreateFiber, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlShutdown, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlCheckFlags, MFF_NO_RETURN);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlInitializeWithAttribute, MFF_NO_RETURN);
});
