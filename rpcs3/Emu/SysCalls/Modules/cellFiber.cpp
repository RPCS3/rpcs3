#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "cellFiber.h"

Module* cellFiber = nullptr;

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
	cellFiber->Log("cellFiberPpuSelf() -> nullptr"); // TODO

	// returns fiber structure (zero for simple PPU thread)
	return vm::ptr<void>::make(0);
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

void cellFiber_init(Module *pxThis)
{
	cellFiber = pxThis;

	cellFiber->AddFunc(0x55870804, _cellFiberPpuInitialize);

	cellFiber->AddFunc(0x9e25c72d, _cellFiberPpuSchedulerAttributeInitialize);
	cellFiber->AddFunc(0xee3b604d, cellFiberPpuInitializeScheduler);
	cellFiber->AddFunc(0x8b6baa01, cellFiberPpuFinalizeScheduler);
	cellFiber->AddFunc(0x12b1acf0, cellFiberPpuRunFibers);
	cellFiber->AddFunc(0xf6c6900c, cellFiberPpuCheckFlags);
	cellFiber->AddFunc(0xe492a675, cellFiberPpuHasRunnableFiber);

	cellFiber->AddFunc(0xc11f8056, _cellFiberPpuAttributeInitialize);
	cellFiber->AddFunc(0x7c2f4034, cellFiberPpuCreateFiber);
	cellFiber->AddFunc(0xfa8d5f95, cellFiberPpuExit);
	cellFiber->AddFunc(0x0c44f441, cellFiberPpuYield);
	cellFiber->AddFunc(0xa6004249, cellFiberPpuJoinFiber);
	cellFiber->AddFunc(0x5d9a7034, cellFiberPpuSelf);
	cellFiber->AddFunc(0x8afb8356, cellFiberPpuSendSignal);
	cellFiber->AddFunc(0x6c164b3b, cellFiberPpuWaitSignal);
	cellFiber->AddFunc(0xa4599cf3, cellFiberPpuWaitFlag);
	cellFiber->AddFunc(0xb0594b2d, cellFiberPpuGetScheduler);
	cellFiber->AddFunc(0xfbf5fe40, cellFiberPpuSetPriority);
	cellFiber->AddFunc(0xf3e81219, cellFiberPpuCheckStackLimit);

	cellFiber->AddFunc(0x31252ec3, _cellFiberPpuContextAttributeInitialize);
	cellFiber->AddFunc(0x72086315, cellFiberPpuContextInitialize);
	cellFiber->AddFunc(0xb3a48079, cellFiberPpuContextFinalize);
	cellFiber->AddFunc(0xaba1c563, cellFiberPpuContextRun);
	cellFiber->AddFunc(0xd0066b17, cellFiberPpuContextSwitch);
	cellFiber->AddFunc(0x34a81091, cellFiberPpuContextSelf);
	cellFiber->AddFunc(0x01036193, cellFiberPpuContextReturnToThread);
	cellFiber->AddFunc(0xb90c871b, cellFiberPpuContextCheckStackLimit);

	cellFiber->AddFunc(0x081c98be, cellFiberPpuContextRunScheduler);
	cellFiber->AddFunc(0x0a25b6c8, cellFiberPpuContextEnterScheduler);

	cellFiber->AddFunc(0xbf9cd933, cellFiberPpuSchedulerTraceInitialize);
	cellFiber->AddFunc(0x3860a12a, cellFiberPpuSchedulerTraceFinalize);
	cellFiber->AddFunc(0xadedbebf, cellFiberPpuSchedulerTraceStart);
	cellFiber->AddFunc(0xe665f9a9, cellFiberPpuSchedulerTraceStop);

	cellFiber->AddFunc(0x68ba4568, _cellFiberPpuUtilWorkerControlAttributeInitialize);
	cellFiber->AddFunc(0x1e7a247a, cellFiberPpuUtilWorkerControlRunFibers);
	cellFiber->AddFunc(0x3204b146, cellFiberPpuUtilWorkerControlInitialize);
	cellFiber->AddFunc(0x392c5aa5, cellFiberPpuUtilWorkerControlSetPollingMode);
	cellFiber->AddFunc(0x3b417f82, cellFiberPpuUtilWorkerControlJoinFiber);
	cellFiber->AddFunc(0x4fc86b2c, cellFiberPpuUtilWorkerControlDisconnectEventQueue);
	cellFiber->AddFunc(0x5d3992dd, cellFiberPpuUtilWorkerControlSendSignal);
	cellFiber->AddFunc(0x62a20f0d, cellFiberPpuUtilWorkerControlConnectEventQueueToSpurs);
	cellFiber->AddFunc(0xa27c95ca, cellFiberPpuUtilWorkerControlFinalize);
	cellFiber->AddFunc(0xbabf714b, cellFiberPpuUtilWorkerControlWakeup);
	cellFiber->AddFunc(0xbfca88d3, cellFiberPpuUtilWorkerControlCreateFiber);
	cellFiber->AddFunc(0xc04e2438, cellFiberPpuUtilWorkerControlShutdown);
	cellFiber->AddFunc(0xea6dc1ad, cellFiberPpuUtilWorkerControlCheckFlags);
	cellFiber->AddFunc(0xf2ccad4f, cellFiberPpuUtilWorkerControlInitializeWithAttribute);
}
