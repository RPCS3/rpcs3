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
	REG_FUNC(cellFiber, _cellFiberPpuInitialize);

	REG_FUNC(cellFiber, _cellFiberPpuSchedulerAttributeInitialize);
	REG_FUNC(cellFiber, cellFiberPpuInitializeScheduler);
	REG_FUNC(cellFiber, cellFiberPpuFinalizeScheduler);
	REG_FUNC(cellFiber, cellFiberPpuRunFibers);
	REG_FUNC(cellFiber, cellFiberPpuCheckFlags);
	REG_FUNC(cellFiber, cellFiberPpuHasRunnableFiber);

	REG_FUNC(cellFiber, _cellFiberPpuAttributeInitialize);
	REG_FUNC(cellFiber, cellFiberPpuCreateFiber);
	REG_FUNC(cellFiber, cellFiberPpuExit);
	REG_FUNC(cellFiber, cellFiberPpuYield);
	REG_FUNC(cellFiber, cellFiberPpuJoinFiber);
	REG_FUNC(cellFiber, cellFiberPpuSelf);
	REG_FUNC(cellFiber, cellFiberPpuSendSignal);
	REG_FUNC(cellFiber, cellFiberPpuWaitSignal);
	REG_FUNC(cellFiber, cellFiberPpuWaitFlag);
	REG_FUNC(cellFiber, cellFiberPpuGetScheduler);
	REG_FUNC(cellFiber, cellFiberPpuSetPriority);
	REG_FUNC(cellFiber, cellFiberPpuCheckStackLimit);

	REG_FUNC(cellFiber, _cellFiberPpuContextAttributeInitialize);
	REG_FUNC(cellFiber, cellFiberPpuContextInitialize);
	REG_FUNC(cellFiber, cellFiberPpuContextFinalize);
	REG_FUNC(cellFiber, cellFiberPpuContextRun);
	REG_FUNC(cellFiber, cellFiberPpuContextSwitch);
	REG_FUNC(cellFiber, cellFiberPpuContextSelf);
	REG_FUNC(cellFiber, cellFiberPpuContextReturnToThread);
	REG_FUNC(cellFiber, cellFiberPpuContextCheckStackLimit);

	REG_FUNC(cellFiber, cellFiberPpuContextRunScheduler);
	REG_FUNC(cellFiber, cellFiberPpuContextEnterScheduler);

	REG_FUNC(cellFiber, cellFiberPpuSchedulerTraceInitialize);
	REG_FUNC(cellFiber, cellFiberPpuSchedulerTraceFinalize);
	REG_FUNC(cellFiber, cellFiberPpuSchedulerTraceStart);
	REG_FUNC(cellFiber, cellFiberPpuSchedulerTraceStop);

	REG_FUNC(cellFiber, _cellFiberPpuUtilWorkerControlAttributeInitialize);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlRunFibers);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlInitialize);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlSetPollingMode);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlJoinFiber);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlDisconnectEventQueue);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlSendSignal);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlConnectEventQueueToSpurs);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlFinalize);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlWakeup);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlCreateFiber);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlShutdown);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlCheckFlags);
	REG_FUNC(cellFiber, cellFiberPpuUtilWorkerControlInitializeWithAttribute);
});
