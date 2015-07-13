#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "cellFiber.h"

extern Module cellFiber;

int _cellFiberPpuInitialize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int _cellFiberPpuSchedulerAttributeInitialize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuInitializeScheduler()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuFinalizeScheduler()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuRunFibers()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuCheckFlags()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuHasRunnableFiber()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int _cellFiberPpuAttributeInitialize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuCreateFiber()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuExit()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuYield()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuJoinFiber()
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

int cellFiberPpuSendSignal()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuWaitSignal()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuWaitFlag()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuGetScheduler()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuSetPriority()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuCheckStackLimit()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int _cellFiberPpuContextAttributeInitialize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuContextInitialize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuContextFinalize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuContextRun()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuContextSwitch()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuContextSelf()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuContextReturnToThread()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuContextCheckStackLimit()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuContextRunScheduler()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuContextEnterScheduler()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuSchedulerTraceInitialize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuSchedulerTraceFinalize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuSchedulerTraceStart()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuSchedulerTraceStop()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int _cellFiberPpuUtilWorkerControlAttributeInitialize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuUtilWorkerControlRunFibers()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuUtilWorkerControlInitialize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuUtilWorkerControlSetPollingMode()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuUtilWorkerControlJoinFiber()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuUtilWorkerControlDisconnectEventQueue()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuUtilWorkerControlSendSignal()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuUtilWorkerControlConnectEventQueueToSpurs()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuUtilWorkerControlFinalize()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuUtilWorkerControlWakeup()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuUtilWorkerControlCreateFiber()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuUtilWorkerControlShutdown()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuUtilWorkerControlCheckFlags()
{
	UNIMPLEMENTED_FUNC(cellFiber);
	return CELL_OK;
}

int cellFiberPpuUtilWorkerControlInitializeWithAttribute()
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
