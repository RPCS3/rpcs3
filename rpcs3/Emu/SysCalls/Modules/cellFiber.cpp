#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "cellFiber.h"

extern Module cellFiber;

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

Module cellFiber("cellFiber", []()
{
	REG_FUNC_NR(cellFiber, _cellFiberPpuInitialize);

	REG_FUNC_NR(cellFiber, _cellFiberPpuSchedulerAttributeInitialize);
	REG_FUNC_NR(cellFiber, cellFiberPpuInitializeScheduler);
	REG_FUNC_NR(cellFiber, cellFiberPpuFinalizeScheduler);
	REG_FUNC_NR(cellFiber, cellFiberPpuRunFibers);
	REG_FUNC_NR(cellFiber, cellFiberPpuCheckFlags);
	REG_FUNC_NR(cellFiber, cellFiberPpuHasRunnableFiber);

	REG_FUNC_NR(cellFiber, _cellFiberPpuAttributeInitialize);
	REG_FUNC_NR(cellFiber, cellFiberPpuCreateFiber);
	REG_FUNC_NR(cellFiber, cellFiberPpuExit);
	REG_FUNC_NR(cellFiber, cellFiberPpuYield);
	REG_FUNC_NR(cellFiber, cellFiberPpuJoinFiber);
	REG_FUNC_NR(cellFiber, cellFiberPpuSelf);
	REG_FUNC_NR(cellFiber, cellFiberPpuSendSignal);
	REG_FUNC_NR(cellFiber, cellFiberPpuWaitSignal);
	REG_FUNC_NR(cellFiber, cellFiberPpuWaitFlag);
	REG_FUNC_NR(cellFiber, cellFiberPpuGetScheduler);
	REG_FUNC_NR(cellFiber, cellFiberPpuSetPriority);
	REG_FUNC_NR(cellFiber, cellFiberPpuCheckStackLimit);

	REG_FUNC_NR(cellFiber, _cellFiberPpuContextAttributeInitialize);
	REG_FUNC_NR(cellFiber, cellFiberPpuContextInitialize);
	REG_FUNC_NR(cellFiber, cellFiberPpuContextFinalize);
	REG_FUNC_NR(cellFiber, cellFiberPpuContextRun);
	REG_FUNC_NR(cellFiber, cellFiberPpuContextSwitch);
	REG_FUNC_NR(cellFiber, cellFiberPpuContextSelf);
	REG_FUNC_NR(cellFiber, cellFiberPpuContextReturnToThread);
	REG_FUNC_NR(cellFiber, cellFiberPpuContextCheckStackLimit);

	REG_FUNC_NR(cellFiber, cellFiberPpuContextRunScheduler);
	REG_FUNC_NR(cellFiber, cellFiberPpuContextEnterScheduler);

	REG_FUNC_NR(cellFiber, cellFiberPpuSchedulerTraceInitialize);
	REG_FUNC_NR(cellFiber, cellFiberPpuSchedulerTraceFinalize);
	REG_FUNC_NR(cellFiber, cellFiberPpuSchedulerTraceStart);
	REG_FUNC_NR(cellFiber, cellFiberPpuSchedulerTraceStop);

	REG_FUNC_NR(cellFiber, _cellFiberPpuUtilWorkerControlAttributeInitialize);
	REG_FUNC_NR(cellFiber, cellFiberPpuUtilWorkerControlRunFibers);
	REG_FUNC_NR(cellFiber, cellFiberPpuUtilWorkerControlInitialize);
	REG_FUNC_NR(cellFiber, cellFiberPpuUtilWorkerControlSetPollingMode);
	REG_FUNC_NR(cellFiber, cellFiberPpuUtilWorkerControlJoinFiber);
	REG_FUNC_NR(cellFiber, cellFiberPpuUtilWorkerControlDisconnectEventQueue);
	REG_FUNC_NR(cellFiber, cellFiberPpuUtilWorkerControlSendSignal);
	REG_FUNC_NR(cellFiber, cellFiberPpuUtilWorkerControlConnectEventQueueToSpurs);
	REG_FUNC_NR(cellFiber, cellFiberPpuUtilWorkerControlFinalize);
	REG_FUNC_NR(cellFiber, cellFiberPpuUtilWorkerControlWakeup);
	REG_FUNC_NR(cellFiber, cellFiberPpuUtilWorkerControlCreateFiber);
	REG_FUNC_NR(cellFiber, cellFiberPpuUtilWorkerControlShutdown);
	REG_FUNC_NR(cellFiber, cellFiberPpuUtilWorkerControlCheckFlags);
	REG_FUNC_NR(cellFiber, cellFiberPpuUtilWorkerControlInitializeWithAttribute);
});
