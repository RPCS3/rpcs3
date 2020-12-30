#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellFiber.h"

LOG_CHANNEL(cellFiber);

template <>
void fmt_class_string<CellFiberError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellFiberError value)
	{
		switch (value)
		{
			STR_CASE(CELL_FIBER_ERROR_AGAIN);
			STR_CASE(CELL_FIBER_ERROR_INVAL);
			STR_CASE(CELL_FIBER_ERROR_NOMEM);
			STR_CASE(CELL_FIBER_ERROR_DEADLK);
			STR_CASE(CELL_FIBER_ERROR_PERM);
			STR_CASE(CELL_FIBER_ERROR_BUSY);
			STR_CASE(CELL_FIBER_ERROR_ABORT);
			STR_CASE(CELL_FIBER_ERROR_STAT);
			STR_CASE(CELL_FIBER_ERROR_ALIGN);
			STR_CASE(CELL_FIBER_ERROR_NULL_POINTER);
			STR_CASE(CELL_FIBER_ERROR_NOSYSINIT);
		}

		return unknown;
	});
}

error_code _cellFiberPpuInitialize()
{
	cellFiber.todo("_cellFiberPpuInitialize()");

	return CELL_OK;
}

error_code _cellFiberPpuSchedulerAttributeInitialize(vm::ptr<CellFiberPpuSchedulerAttribute> attr, u32 sdkVersion)
{
	cellFiber.warning("_cellFiberPpuSchedulerAttributeInitialize(attr=*0x%x, sdkVersion=0x%x)", attr, sdkVersion);

	if (!attr)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	memset(attr.get_ptr(), 0, sizeof(CellFiberPpuSchedulerAttribute));
	attr->autoCheckFlags = false;
	attr->autoCheckFlagsIntervalUsec = 0;
	attr->debuggerSupport = false;

	return CELL_OK;
}

error_code cellFiberPpuInitializeScheduler(vm::ptr<CellFiberPpuScheduler> scheduler, vm::ptr<CellFiberPpuSchedulerAttribute> attr)
{
	cellFiber.todo("cellFiberPpuInitializeScheduler(scheduler=*0x%x, attr=*0x%x)", scheduler, attr);

	if (!scheduler)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!scheduler.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuFinalizeScheduler(vm::ptr<CellFiberPpuScheduler> scheduler)
{
	cellFiber.todo("cellFiberPpuFinalizeScheduler(scheduler=*0x%x)", scheduler);

	if (!scheduler)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!scheduler.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuRunFibers(vm::ptr<CellFiberPpuScheduler> scheduler)
{
	cellFiber.todo("cellFiberPpuRunFibers(scheduler=*0x%x)", scheduler);

	if (!scheduler)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!scheduler.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuCheckFlags(vm::ptr<CellFiberPpuScheduler> scheduler)
{
	cellFiber.todo("cellFiberPpuCheckFlags(scheduler=*0x%x)", scheduler);

	if (!scheduler)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!scheduler.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuHasRunnableFiber(vm::ptr<CellFiberPpuScheduler> scheduler, vm::ptr<b8> flag)
{
	cellFiber.todo("cellFiberPpuHasRunnableFiber(scheduler=*0x%x, flag=*0x%x)", scheduler, flag);

	if (!scheduler || !flag)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!scheduler.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code _cellFiberPpuAttributeInitialize(vm::ptr<CellFiberPpuAttribute> attr, u32 sdkVersion)
{
	cellFiber.warning("_cellFiberPpuAttributeInitialize(attr=*0x%x, sdkVersion=0x%x)", attr, sdkVersion);

	if (!attr)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	memset(attr.get_ptr(), 0, sizeof(CellFiberPpuAttribute));
	attr->onExitCallback = vm::null;

	return CELL_OK;
}

error_code cellFiberPpuCreateFiber(vm::ptr<CellFiberPpuScheduler> scheduler, vm::ptr<CellFiberPpu> fiber, vm::ptr<CellFiberPpuEntry> entry, u64 arg, u32 priority, vm::ptr<void> eaStack, u32 sizeStack, vm::cptr<CellFiberPpuAttribute> attr)
{
	cellFiber.todo("cellFiberPpuCreateFiber(scheduler=*0x%x, fiber=*0x%x, entry=*0x%x, arg=0x%x, priority=%d, eaStack=*0x%x, sizeStack=0x%x, attr=*0x%x)", scheduler, fiber, entry, arg, priority, eaStack, sizeStack, attr);

	if (!scheduler || !fiber || !entry || !eaStack)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!scheduler.aligned() || !fiber.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	if (priority > 3)
	{
		return CELL_FIBER_ERROR_INVAL;
	}

	return CELL_OK;
}

error_code cellFiberPpuExit(s32 status)
{
	cellFiber.todo("cellFiberPpuExit(status=%d)", status);

	return CELL_OK;
}

error_code cellFiberPpuYield()
{
	cellFiber.todo("cellFiberPpuYield()");

	return CELL_OK;
}

error_code cellFiberPpuJoinFiber(vm::ptr<CellFiberPpu> fiber, vm::ptr<s32> status)
{
	cellFiber.todo("cellFiberPpuJoinFiber(fiber=*0x%x, status=*0x%x)", fiber, status);

	if (!fiber || !status)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!fiber.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

vm::ptr<void> cellFiberPpuSelf()
{
	cellFiber.trace("cellFiberPpuSelf() -> nullptr"); // TODO

	// returns fiber structure (zero for simple PPU thread)
	return vm::null;
}

error_code cellFiberPpuSendSignal(vm::ptr<CellFiberPpu> fiber, vm::ptr<u32> numWorker)
{
	cellFiber.todo("cellFiberPpuSendSignal(fiber=*0x%x, numWorker=*0x%x)", fiber, numWorker);

	if (!fiber)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!fiber.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuWaitSignal()
{
	cellFiber.todo("cellFiberPpuWaitSignal()");

	return CELL_OK;
}

error_code cellFiberPpuWaitFlag(vm::ptr<u32> eaFlag, b8 flagValue)
{
	cellFiber.todo("cellFiberPpuWaitFlag(eaFlag=*0x%x, flagValue=%d)", eaFlag, flagValue);

	if (!eaFlag)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

error_code cellFiberPpuGetScheduler(vm::ptr<CellFiberPpu> fiber, vm::pptr<CellFiberPpuScheduler> pScheduler)
{
	cellFiber.todo("cellFiberPpuGetScheduler(fiber=*0x%x, pScheduler=**0x%x)", fiber, pScheduler);

	if (!fiber || !pScheduler)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!fiber.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuSetPriority(u32 priority)
{
	cellFiber.todo("cellFiberPpuSetPriority(priority=%d)", priority);

	if (priority > 3)
	{
		return CELL_FIBER_ERROR_INVAL;
	}

	return CELL_OK;
}

error_code cellFiberPpuCheckStackLimit()
{
	cellFiber.todo("cellFiberPpuCheckStackLimit()");

	return CELL_OK;
}

error_code _cellFiberPpuContextAttributeInitialize(vm::ptr<CellFiberPpuContextAttribute> attr, u32 sdkVersion)
{
	cellFiber.warning("_cellFiberPpuContextAttributeInitialize(attr=*0x%x, sdkVersion=0x%x)", attr, sdkVersion);

	if (!attr)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	memset(attr.get_ptr(), 0, sizeof(CellFiberPpuContextAttribute));
	attr->debuggerSupport = false;

	return CELL_OK;
}

error_code cellFiberPpuContextInitialize(vm::ptr<CellFiberPpuContext> context, vm::ptr<CellFiberPpuContextEntry> entry, u64 arg, vm::ptr<void> eaStack, u32 sizeStack, vm::cptr<CellFiberPpuContextAttribute> attr)
{
	cellFiber.todo("cellFiberPpuContextInitialize(context=*0x%x, entry=*0x%x, arg=0x%x, eaStack=*0x%x, sizeStack=0x%x, attr=*0x%x)", context, entry, arg, eaStack, sizeStack, attr);

	if (!context || !entry || !eaStack)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!context.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuContextFinalize(vm::ptr<CellFiberPpuContext> context)
{
	cellFiber.todo("cellFiberPpuContextFinalize(context=*0x%x)", context);

	if (!context)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!context.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuContextRun(vm::ptr<CellFiberPpuContext> context, vm::ptr<s32> cause, vm::pptr<CellFiberPpuContext> fiberFrom, vm::cptr<CellFiberPpuContextExecutionOption> option)
{
	cellFiber.todo("cellFiberPpuContextRun(context=*0x%x, cause=*0x%x, fiberFrom=**0x%x, option=*0x%x)", context, cause, fiberFrom, option);

	if (!context || !cause)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!context.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuContextSwitch(vm::ptr<CellFiberPpuContext> context, vm::pptr<CellFiberPpuContext> fiberFrom, vm::cptr<CellFiberPpuContextExecutionOption> option)
{
	cellFiber.todo("cellFiberPpuContextSwitch(context=*0x%x, fiberFrom=**0x%x, option=*0x%x)", context, fiberFrom, option);

	if (!context)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!context.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

vm::ptr<CellFiberPpuContext> cellFiberPpuContextSelf()
{
	cellFiber.todo("cellFiberPpuContextSelf()");

	return vm::null;
}

error_code cellFiberPpuContextReturnToThread(s32 cause)
{
	cellFiber.todo("cellFiberPpuContextReturnToThread(cause=%d)", cause);

	return CELL_OK;
}

error_code cellFiberPpuContextCheckStackLimit()
{
	cellFiber.todo("cellFiberPpuContextCheckStackLimit()");

	return CELL_OK;
}

error_code cellFiberPpuContextRunScheduler(vm::ptr<CellFiberPpuSchedulerCallback> scheduler, u64 arg0, u64 arg1, vm::ptr<s32> cause, vm::pptr<CellFiberPpuContext> fiberFrom, vm::cptr<CellFiberPpuContextExecutionOption> option)
{
	cellFiber.todo("cellFiberPpuContextRunScheduler(scheduler=*0x%x, arg0=0x%x, arg1=0x%x, cause=*0x%x, fiberFrom=**0x%x, option=*0x%x)", scheduler, arg0, arg1, cause, fiberFrom, option);

	if (!scheduler || !cause)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

error_code cellFiberPpuContextEnterScheduler(vm::ptr<CellFiberPpuSchedulerCallback> scheduler, u64 arg0, u64 arg1, vm::pptr<CellFiberPpuContext> fiberFrom, vm::cptr<CellFiberPpuContextExecutionOption> option)
{
	cellFiber.todo("cellFiberPpuContextEnterScheduler(scheduler=*0x%x, arg0=0x%x, arg1=0x%x, fiberFrom=**0x%x, option=*0x%x)", scheduler, arg0, arg1, fiberFrom, option);

	if (!scheduler)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

error_code cellFiberPpuSchedulerTraceInitialize(vm::ptr<CellFiberPpuScheduler> scheduler, vm::ptr<void> buffer, u32 size, u32 mode)
{
	cellFiber.todo("cellFiberPpuSchedulerTraceInitialize(scheduler=*0x%x, buffer=*0x%x, size=0x%x, mode=0x%x)", scheduler, buffer, size, mode);

	if (!scheduler || !buffer)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!scheduler.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuSchedulerTraceFinalize(vm::ptr<CellFiberPpuScheduler> scheduler)
{
	cellFiber.todo("cellFiberPpuSchedulerTraceFinalize(scheduler=*0x%x)", scheduler);

	if (!scheduler)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!scheduler.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuSchedulerTraceStart(vm::ptr<CellFiberPpuScheduler> scheduler)
{
	cellFiber.todo("cellFiberPpuSchedulerTraceStart(scheduler=*0x%x)", scheduler);

	if (!scheduler)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!scheduler.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuSchedulerTraceStop(vm::ptr<CellFiberPpuScheduler> scheduler)
{
	cellFiber.todo("cellFiberPpuSchedulerTraceStop(scheduler=*0x%x)", scheduler);

	if (!scheduler)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!scheduler.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code _cellFiberPpuUtilWorkerControlAttributeInitialize(vm::ptr<CellFiberPpuUtilWorkerControlAttribute> attr, u32 sdkVersion)
{
	cellFiber.warning("_cellFiberPpuUtilWorkerControlAttributeInitialize(attr=*0x%x, sdkVersion=0x%x)", attr, sdkVersion);

	if (!attr)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!attr.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	memset(attr.get_ptr(), 0, sizeof(CellFiberPpuUtilWorkerControlAttribute));
	attr->scheduler.autoCheckFlags = false;
	attr->scheduler.autoCheckFlagsIntervalUsec = 0;
	attr->scheduler.debuggerSupport = false;

	return CELL_OK;
}

error_code cellFiberPpuUtilWorkerControlRunFibers(vm::ptr<CellFiberPpuUtilWorkerControl> control)
{
	cellFiber.todo("cellFiberPpuUtilWorkerControlRunFibers(control=*0x%x)", control);

	if (!control)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!control.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuUtilWorkerControlInitialize(vm::ptr<CellFiberPpuUtilWorkerControl> control)
{
	cellFiber.todo("cellFiberPpuUtilWorkerControlInitialize(control=*0x%x)", control);

	if (!control)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!control.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuUtilWorkerControlSetPollingMode(vm::ptr<CellFiberPpuUtilWorkerControl> control, s32 mode, s32 timeout)
{
	cellFiber.todo("cellFiberPpuUtilWorkerControlSetPollingMode(control=*0x%x, mode=%d, timeout=%d)", control, mode, timeout);

	if (!control)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!control.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuUtilWorkerControlJoinFiber(vm::ptr<CellFiberPpuUtilWorkerControl> control, vm::ptr<CellFiberPpu> fiber, vm::ptr<s32> exitCode)
{
	cellFiber.todo("cellFiberPpuUtilWorkerControlJoinFiber(control=*0x%x, fiber=*0x%x, exitCode=*0x%x)", control, fiber, exitCode);

	if (!control)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!control.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuUtilWorkerControlDisconnectEventQueue()
{
	cellFiber.todo("cellFiberPpuUtilWorkerControlDisconnectEventQueue()");

	return CELL_OK;
}

error_code cellFiberPpuUtilWorkerControlSendSignal(vm::ptr<CellFiberPpu> fiber, vm::ptr<u32> numWorker)
{
	cellFiber.todo("cellFiberPpuUtilWorkerControlSendSignal(fiber=*0x%x, numWorker=*0x%x)", fiber, numWorker);

	if (!fiber)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!fiber.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuUtilWorkerControlConnectEventQueueToSpurs()
{
	cellFiber.todo("cellFiberPpuUtilWorkerControlConnectEventQueueToSpurs()");

	return CELL_OK;
}

error_code cellFiberPpuUtilWorkerControlFinalize(vm::ptr<CellFiberPpuUtilWorkerControl> control)
{
	cellFiber.todo("cellFiberPpuUtilWorkerControlFinalize(control=*0x%x)", control);

	if (!control)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!control.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuUtilWorkerControlWakeup(vm::ptr<CellFiberPpuUtilWorkerControl> control)
{
	cellFiber.todo("cellFiberPpuUtilWorkerControlWakeup(control=*0x%x)", control);

	if (!control)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!control.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuUtilWorkerControlCreateFiber(vm::ptr<CellFiberPpuUtilWorkerControl> control, vm::ptr<CellFiberPpu> fiber, vm::ptr<CellFiberPpuEntry> entry, u64 arg, u32 priority, vm::ptr<void> eaStack, u32 sizeStack, vm::cptr<CellFiberPpuAttribute> attr)
{
	cellFiber.todo("cellFiberPpuUtilWorkerControlCreateFiber(control=*0x%x, fiber=*0x%x, entry=*0x%x, arg=0x%x, priority=%d, eaStack=*0x%x, sizeStack=0x%x, attr=*0x%x)", control, fiber, entry, arg, priority, eaStack, sizeStack, attr);

	if (!control)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!control.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuUtilWorkerControlShutdown(vm::ptr<CellFiberPpuUtilWorkerControl> control)
{
	cellFiber.todo("cellFiberPpuUtilWorkerControlShutdown(control=*0x%x)", control);

	if (!control)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!control.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuUtilWorkerControlCheckFlags(vm::ptr<CellFiberPpuUtilWorkerControl> control, b8 wakingUp)
{
	cellFiber.todo("cellFiberPpuUtilWorkerControlCheckFlags(control=*0x%x, wakingUp=%d)", control, wakingUp);

	if (!control)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!control.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

	return CELL_OK;
}

error_code cellFiberPpuUtilWorkerControlInitializeWithAttribute(vm::ptr<CellFiberPpuUtilWorkerControl> control, vm::ptr<CellFiberPpuUtilWorkerControlAttribute> attr)
{
	cellFiber.todo("cellFiberPpuUtilWorkerControlInitializeWithAttribute(control=*0x%x, attr=*0x%x)", control, attr);

	if (!control)
	{
		return CELL_FIBER_ERROR_NULL_POINTER;
	}

	if (!control.aligned())
	{
		return CELL_FIBER_ERROR_ALIGN;
	}

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
