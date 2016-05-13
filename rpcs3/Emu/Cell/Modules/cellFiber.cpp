#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "cellFiber.h"

logs::channel cellFiber("cellFiber", logs::level::notice);

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
	cellFiber.trace("cellFiberPpuSelf() -> nullptr"); // TODO

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

DECLARE(ppu_module_manager::cellFiber)("cellFiber", []()
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
