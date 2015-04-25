#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"
#include "Emu/ARMv7/PSVObjectList.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/SysCalls/Callback.h"
#include "Emu/ARMv7/ARMv7Thread.h"

#include "sceLibKernel.h"
#include "psv_sema.h"
#include "psv_event_flag.h"
#include "psv_mutex.h"
#include "psv_cond.h"

#define RETURN_ERROR(code) { Emu.Pause(); sceLibKernel.Error("%s() failed: %s", __FUNCTION__, #code); return code; }

s32 sceKernelAllocMemBlock(vm::psv::ptr<const char> name, s32 type, u32 vsize, vm::psv::ptr<SceKernelAllocMemBlockOpt> pOpt)
{
	throw __FUNCTION__;
}

s32 sceKernelFreeMemBlock(s32 uid)
{
	throw __FUNCTION__;
}

s32 sceKernelGetMemBlockBase(s32 uid, vm::psv::ptr<vm::psv::ptr<void>> ppBase)
{
	throw __FUNCTION__;
}

s32 sceKernelGetMemBlockInfoByAddr(vm::psv::ptr<void> vbase, vm::psv::ptr<SceKernelMemBlockInfo> pInfo)
{
	throw __FUNCTION__;
}

s32 sceKernelCreateThread(
	vm::psv::ptr<const char> pName,
	vm::psv::ptr<SceKernelThreadEntry> entry,
	s32 initPriority,
	u32 stackSize,
	u32 attr,
	s32 cpuAffinityMask,
	vm::psv::ptr<const SceKernelThreadOptParam> pOptParam)
{
	sceLibKernel.Warning("sceKernelCreateThread(pName=*0x%x, entry=*0x%x, initPriority=%d, stackSize=0x%x, attr=0x%x, cpuAffinityMask=0x%x, pOptParam=*0x%x)",
		pName, entry, initPriority, stackSize, attr, cpuAffinityMask, pOptParam);

	auto t = Emu.GetCPU().AddThread(CPU_THREAD_ARMv7);

	auto& armv7 = static_cast<ARMv7Thread&>(*t);

	armv7.SetEntry(entry.addr());
	armv7.SetPrio(initPriority);
	armv7.SetStackSize(stackSize);
	armv7.SetName(pName.get_ptr());
	armv7.Run();

	return armv7.GetId();
}

s32 sceKernelStartThread(s32 threadId, u32 argSize, vm::psv::ptr<const void> pArgBlock)
{
	sceLibKernel.Warning("sceKernelStartThread(threadId=0x%x, argSize=0x%x, pArgBlock=*0x%x)", threadId, argSize, pArgBlock);

	const auto t = Emu.GetCPU().GetThread(threadId, CPU_THREAD_ARMv7);

	if (!t)
	{
		RETURN_ERROR(SCE_KERNEL_ERROR_INVALID_UID);
	}

	// thread should be in DORMANT state, but it's not possible to check it correctly atm

	if (t->IsAlive())
	{
		RETURN_ERROR(SCE_KERNEL_ERROR_NOT_DORMANT);
	}

	ARMv7Thread& thread = static_cast<ARMv7Thread&>(*t);

	// push arg block onto the stack
	const u32 pos = (thread.context.SP -= argSize);
	memcpy(vm::get_ptr<void>(pos), pArgBlock.get_ptr(), argSize);

	// set SceKernelThreadEntry function arguments
	thread.context.GPR[0] = argSize;
	thread.context.GPR[1] = pos;

	thread.Exec();
	return SCE_OK;
}

s32 sceKernelExitThread(ARMv7Context& context, s32 exitStatus)
{
	sceLibKernel.Warning("sceKernelExitThread(exitStatus=0x%x)", exitStatus);

	// exit status is stored in r0
	context.thread.Stop();

	return SCE_OK;
}

s32 sceKernelDeleteThread(s32 threadId)
{
	sceLibKernel.Warning("sceKernelDeleteThread(threadId=0x%x)", threadId);

	const auto t = Emu.GetCPU().GetThread(threadId, CPU_THREAD_ARMv7);

	if (!t)
	{
		RETURN_ERROR(SCE_KERNEL_ERROR_INVALID_UID);
	}

	// thread should be in DORMANT state, but it's not possible to check it correctly atm

	if (t->IsAlive())
	{
		RETURN_ERROR(SCE_KERNEL_ERROR_NOT_DORMANT);
	}

	Emu.GetCPU().RemoveThread(threadId);
	return SCE_OK;
}

s32 sceKernelExitDeleteThread(ARMv7Context& context, s32 exitStatus)
{
	sceLibKernel.Warning("sceKernelExitDeleteThread(exitStatus=0x%x)", exitStatus);

	// exit status is stored in r0
	context.thread.Stop();

	// current thread should be deleted
	const u32 id = context.thread.GetId();
	CallAfter([id]()
	{
		Emu.GetCPU().RemoveThread(id);
	});

	return SCE_OK;
}

s32 sceKernelChangeThreadCpuAffinityMask(s32 threadId, s32 cpuAffinityMask)
{
	sceLibKernel.Todo("sceKernelChangeThreadCpuAffinityMask(threadId=0x%x, cpuAffinityMask=0x%x)", threadId, cpuAffinityMask);

	throw __FUNCTION__;
}

s32 sceKernelGetThreadCpuAffinityMask(s32 threadId)
{
	sceLibKernel.Todo("sceKernelGetThreadCpuAffinityMask(threadId=0x%x)", threadId);

	throw __FUNCTION__;
}

s32 sceKernelChangeThreadPriority(s32 threadId, s32 priority)
{
	sceLibKernel.Todo("sceKernelChangeThreadPriority(threadId=0x%x, priority=%d)", threadId, priority);

	throw __FUNCTION__;
}

s32 sceKernelGetThreadCurrentPriority()
{
	sceLibKernel.Todo("sceKernelGetThreadCurrentPriority()");

	throw __FUNCTION__;
}

u32 sceKernelGetThreadId(ARMv7Context& context)
{
	sceLibKernel.Log("sceKernelGetThreadId()");

	return context.thread.GetId();
}

s32 sceKernelChangeCurrentThreadAttr(u32 clearAttr, u32 setAttr)
{
	sceLibKernel.Todo("sceKernelChangeCurrentThreadAttr()");

	throw __FUNCTION__;
}

s32 sceKernelGetThreadExitStatus(s32 threadId, vm::psv::ptr<s32> pExitStatus)
{
	sceLibKernel.Todo("sceKernelGetThreadExitStatus(threadId=0x%x, pExitStatus=*0x%x)", threadId, pExitStatus);

	throw __FUNCTION__;
}

s32 sceKernelGetProcessId()
{
	sceLibKernel.Todo("sceKernelGetProcessId()");

	throw __FUNCTION__;
}

s32 sceKernelCheckWaitableStatus()
{
	sceLibKernel.Todo("sceKernelCheckWaitableStatus()");

	throw __FUNCTION__;
}

s32 sceKernelGetThreadInfo(s32 threadId, vm::psv::ptr<SceKernelThreadInfo> pInfo)
{
	sceLibKernel.Todo("sceKernelGetThreadInfo(threadId=0x%x, pInfo=*0x%x)", threadId, pInfo);

	throw __FUNCTION__;
}

s32 sceKernelGetThreadRunStatus(vm::psv::ptr<SceKernelThreadRunStatus> pStatus)
{
	sceLibKernel.Todo("sceKernelGetThreadRunStatus(pStatus=*0x%x)", pStatus);

	throw __FUNCTION__;
}

s32 sceKernelGetSystemInfo(vm::psv::ptr<SceKernelSystemInfo> pInfo)
{
	sceLibKernel.Todo("sceKernelGetSystemInfo(pInfo=*0x%x)", pInfo);

	throw __FUNCTION__;
}

s32 sceKernelGetThreadmgrUIDClass(s32 uid)
{
	sceLibKernel.Todo("sceKernelGetThreadmgrUIDClass(uid=0x%x)", uid);

	throw __FUNCTION__;
}

s32 sceKernelChangeThreadVfpException(s32 clearMask, s32 setMask)
{
	sceLibKernel.Todo("sceKernelChangeThreadVfpException(clearMask=0x%x, setMask=0x%x)", clearMask, setMask);

	throw __FUNCTION__;
}

s32 sceKernelGetCurrentThreadVfpException()
{
	sceLibKernel.Todo("sceKernelGetCurrentThreadVfpException()");

	throw __FUNCTION__;
}

s32 sceKernelDelayThread(u32 usec)
{
	sceLibKernel.Todo("sceKernelDelayThread()");

	throw __FUNCTION__;
}

s32 sceKernelDelayThreadCB(u32 usec)
{
	sceLibKernel.Todo("sceKernelDelayThreadCB()");

	throw __FUNCTION__;
}

s32 sceKernelWaitThreadEnd(s32 threadId, vm::psv::ptr<s32> pExitStatus, vm::psv::ptr<u32> pTimeout)
{
	sceLibKernel.Warning("sceKernelWaitThreadEnd(threadId=0x%x, pExitStatus=*0x%x, pTimeout=*0x%x)", threadId, pExitStatus, pTimeout);

	const auto t = Emu.GetCPU().GetThread(threadId, CPU_THREAD_ARMv7);

	if (!t)
	{
		RETURN_ERROR(SCE_KERNEL_ERROR_INVALID_UID);
	}

	ARMv7Thread& thread = static_cast<ARMv7Thread&>(*t);

	if (pTimeout)
	{
	}

	while (thread.IsAlive())
	{
		if (Emu.IsStopped())
		{
			sceLibKernel.Warning("sceKernelWaitThreadEnd(0x%x) aborted", threadId);
			return SCE_OK;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
	}

	if (pExitStatus)
	{
		*pExitStatus = thread.context.GPR[0];
	}

	return SCE_OK;
}

s32 sceKernelWaitThreadEndCB(s32 threadId, vm::psv::ptr<s32> pExitStatus, vm::psv::ptr<u32> pTimeout)
{
	sceLibKernel.Todo("sceKernelWaitThreadEndCB(threadId=0x%x, pExitStatus=*0x%x, pTimeout=*0x%x)", threadId, pExitStatus, pTimeout);

	throw __FUNCTION__;
}

// Callback functions

s32 sceKernelCreateCallback(vm::psv::ptr<const char> pName, u32 attr, vm::psv::ptr<SceKernelCallbackFunction> callbackFunc, vm::psv::ptr<void> pCommon)
{
	throw __FUNCTION__;
}

s32 sceKernelDeleteCallback(s32 callbackId)
{
	throw __FUNCTION__;
}

s32 sceKernelNotifyCallback(s32 callbackId, s32 notifyArg)
{
	throw __FUNCTION__;
}

s32 sceKernelCancelCallback(s32 callbackId)
{
	throw __FUNCTION__;
}

s32 sceKernelGetCallbackCount(s32 callbackId)
{
	throw __FUNCTION__;
}

s32 sceKernelCheckCallback()
{
	throw __FUNCTION__;
}

s32 sceKernelGetCallbackInfo(s32 callbackId, vm::psv::ptr<SceKernelCallbackInfo> pInfo)
{
	throw __FUNCTION__;
}

s32 sceKernelRegisterCallbackToEvent(s32 eventId, s32 callbackId)
{
	throw __FUNCTION__;
}

s32 sceKernelUnregisterCallbackFromEvent(s32 eventId, s32 callbackId)
{
	throw __FUNCTION__;
}

s32 sceKernelUnregisterCallbackFromEventAll(s32 eventId)
{
	throw __FUNCTION__;
}

// Event functions

s32 sceKernelWaitEvent(s32 eventId, u32 waitPattern, vm::psv::ptr<u32> pResultPattern, vm::psv::ptr<u64> pUserData, vm::psv::ptr<u32> pTimeout)
{
	throw __FUNCTION__;
}

s32 sceKernelWaitEventCB(s32 eventId, u32 waitPattern, vm::psv::ptr<u32> pResultPattern, vm::psv::ptr<u64> pUserData, vm::psv::ptr<u32> pTimeout)
{
	throw __FUNCTION__;
}

s32 sceKernelPollEvent(s32 eventId, u32 bitPattern, vm::psv::ptr<u32> pResultPattern, vm::psv::ptr<u64> pUserData)
{
	throw __FUNCTION__;
}

s32 sceKernelCancelEvent(s32 eventId, vm::psv::ptr<s32> pNumWaitThreads)
{
	throw __FUNCTION__;
}

s32 sceKernelGetEventInfo(s32 eventId, vm::psv::ptr<SceKernelEventInfo> pInfo)
{
	throw __FUNCTION__;
}

s32 sceKernelWaitMultipleEvents(vm::psv::ptr<SceKernelWaitEvent> pWaitEventList, s32 numEvents, u32 waitMode, vm::psv::ptr<SceKernelResultEvent> pResultEventList, vm::psv::ptr<u32> pTimeout)
{
	throw __FUNCTION__;
}

s32 sceKernelWaitMultipleEventsCB(vm::psv::ptr<SceKernelWaitEvent> pWaitEventList, s32 numEvents, u32 waitMode, vm::psv::ptr<SceKernelResultEvent> pResultEventList, vm::psv::ptr<u32> pTimeout)
{
	throw __FUNCTION__;
}

// Event flag functions

s32 sceKernelCreateEventFlag(vm::psv::ptr<const char> pName, u32 attr, u32 initPattern, vm::psv::ptr<const SceKernelEventFlagOptParam> pOptParam)
{
	sceLibKernel.Error("sceKernelCreateEventFlag(pName=*0x%x, attr=0x%x, initPattern=0x%x, pOptParam=*0x%x)", pName, attr, initPattern, pOptParam);

	if (s32 id = g_psv_ef_list.add(new psv_event_flag_t(pName.get_ptr(), attr, initPattern), 0))
	{
		return id;
	}

	RETURN_ERROR(SCE_KERNEL_ERROR_ERROR);
}

s32 sceKernelDeleteEventFlag(s32 evfId)
{
	throw __FUNCTION__;
}

s32 sceKernelOpenEventFlag(vm::psv::ptr<const char> pName)
{
	throw __FUNCTION__;
}

s32 sceKernelCloseEventFlag(s32 evfId)
{
	throw __FUNCTION__;
}

s32 sceKernelWaitEventFlag(s32 evfId, u32 bitPattern, u32 waitMode, vm::psv::ptr<u32> pResultPat, vm::psv::ptr<u32> pTimeout)
{
	throw __FUNCTION__;
}

s32 sceKernelWaitEventFlagCB(s32 evfId, u32 bitPattern, u32 waitMode, vm::psv::ptr<u32> pResultPat, vm::psv::ptr<u32> pTimeout)
{
	throw __FUNCTION__;
}

s32 sceKernelPollEventFlag(s32 evfId, u32 bitPattern, u32 waitMode, vm::psv::ptr<u32> pResultPat)
{
	throw __FUNCTION__;
}

s32 sceKernelSetEventFlag(s32 evfId, u32 bitPattern)
{
	throw __FUNCTION__;
}

s32 sceKernelClearEventFlag(s32 evfId, u32 bitPattern)
{
	throw __FUNCTION__;
}

s32 sceKernelCancelEventFlag(s32 evfId, u32 setPattern, vm::psv::ptr<s32> pNumWaitThreads)
{
	throw __FUNCTION__;
}

s32 sceKernelGetEventFlagInfo(s32 evfId, vm::psv::ptr<SceKernelEventFlagInfo> pInfo)
{
	throw __FUNCTION__;
}

// Semaphore functions

s32 sceKernelCreateSema(vm::psv::ptr<const char> pName, u32 attr, s32 initCount, s32 maxCount, vm::psv::ptr<const SceKernelSemaOptParam> pOptParam)
{
	sceLibKernel.Error("sceKernelCreateSema(pName=*0x%x, attr=0x%x, initCount=%d, maxCount=%d, pOptParam=*0x%x)", pName, attr, initCount, maxCount, pOptParam);

	if (s32 id = g_psv_sema_list.add(new psv_sema_t(pName.get_ptr(), attr, initCount, maxCount), 0))
	{
		return id;
	}

	RETURN_ERROR(SCE_KERNEL_ERROR_ERROR);
}

s32 sceKernelDeleteSema(s32 semaId)
{
	sceLibKernel.Error("sceKernelDeleteSema(semaId=0x%x)", semaId);

	ref_t<psv_sema_t> sema = g_psv_sema_list.get(semaId);

	if (!sema)
	{
		RETURN_ERROR(SCE_KERNEL_ERROR_INVALID_UID);
	}

	if (!g_psv_sema_list.remove(semaId))
	{
		RETURN_ERROR(SCE_KERNEL_ERROR_INVALID_UID);
	}

	return SCE_OK;
}

s32 sceKernelOpenSema(vm::psv::ptr<const char> pName)
{
	throw __FUNCTION__;
}

s32 sceKernelCloseSema(s32 semaId)
{
	throw __FUNCTION__;
}

s32 sceKernelWaitSema(s32 semaId, s32 needCount, vm::psv::ptr<u32> pTimeout)
{
	sceLibKernel.Error("sceKernelWaitSema(semaId=0x%x, needCount=%d, pTimeout=*0x%x)", semaId, needCount, pTimeout);

	ref_t<psv_sema_t> sema = g_psv_sema_list.get(semaId);

	if (!sema)
	{
		RETURN_ERROR(SCE_KERNEL_ERROR_INVALID_UID);
	}

	sceLibKernel.Error("*** name = %s", sema->name);
	Emu.Pause();
	return SCE_OK;
}

s32 sceKernelWaitSemaCB(s32 semaId, s32 needCount, vm::psv::ptr<u32> pTimeout)
{
	throw __FUNCTION__;
}

s32 sceKernelPollSema(s32 semaId, s32 needCount)
{
	throw __FUNCTION__;
}

s32 sceKernelSignalSema(s32 semaId, s32 signalCount)
{
	throw __FUNCTION__;
}

s32 sceKernelCancelSema(s32 semaId, s32 setCount, vm::psv::ptr<s32> pNumWaitThreads)
{
	throw __FUNCTION__;
}

s32 sceKernelGetSemaInfo(s32 semaId, vm::psv::ptr<SceKernelSemaInfo> pInfo)
{
	throw __FUNCTION__;
}

// Mutex functions

s32 sceKernelCreateMutex(vm::psv::ptr<const char> pName, u32 attr, s32 initCount, vm::psv::ptr<const SceKernelMutexOptParam> pOptParam)
{
	sceLibKernel.Error("sceKernelCreateMutex(pName=*0x%x, attr=0x%x, initCount=%d, pOptParam=*0x%x)", pName, attr, initCount, pOptParam);

	if (s32 id = g_psv_mutex_list.add(new psv_mutex_t(pName.get_ptr(), attr, initCount), 0))
	{
		return id;
	}

	RETURN_ERROR(SCE_KERNEL_ERROR_ERROR);
}

s32 sceKernelDeleteMutex(s32 mutexId)
{
	throw __FUNCTION__;
}

s32 sceKernelOpenMutex(vm::psv::ptr<const char> pName)
{
	throw __FUNCTION__;
}

s32 sceKernelCloseMutex(s32 mutexId)
{
	throw __FUNCTION__;
}

s32 sceKernelLockMutex(s32 mutexId, s32 lockCount, vm::psv::ptr<u32> pTimeout)
{
	throw __FUNCTION__;
}

s32 sceKernelLockMutexCB(s32 mutexId, s32 lockCount, vm::psv::ptr<u32> pTimeout)
{
	throw __FUNCTION__;
}

s32 sceKernelTryLockMutex(s32 mutexId, s32 lockCount)
{
	throw __FUNCTION__;
}

s32 sceKernelUnlockMutex(s32 mutexId, s32 unlockCount)
{
	throw __FUNCTION__;
}

s32 sceKernelCancelMutex(s32 mutexId, s32 newCount, vm::psv::ptr<s32> pNumWaitThreads)
{
	throw __FUNCTION__;
}

s32 sceKernelGetMutexInfo(s32 mutexId, vm::psv::ptr<SceKernelMutexInfo> pInfo)
{
	throw __FUNCTION__;
}

// Lightweight mutex functions

s32 sceKernelCreateLwMutex(vm::psv::ptr<SceKernelLwMutexWork> pWork, vm::psv::ptr<const char> pName, u32 attr, s32 initCount, vm::psv::ptr<const SceKernelLwMutexOptParam> pOptParam)
{
	throw __FUNCTION__;
}

s32 sceKernelDeleteLwMutex(vm::psv::ptr<SceKernelLwMutexWork> pWork)
{
	throw __FUNCTION__;
}

s32 sceKernelLockLwMutex(vm::psv::ptr<SceKernelLwMutexWork> pWork, s32 lockCount, vm::psv::ptr<u32> pTimeout)
{
	throw __FUNCTION__;
}

s32 sceKernelLockLwMutexCB(vm::psv::ptr<SceKernelLwMutexWork> pWork, s32 lockCount, vm::psv::ptr<u32> pTimeout)
{
	throw __FUNCTION__;
}

s32 sceKernelTryLockLwMutex(vm::psv::ptr<SceKernelLwMutexWork> pWork, s32 lockCount)
{
	throw __FUNCTION__;
}

s32 sceKernelUnlockLwMutex(vm::psv::ptr<SceKernelLwMutexWork> pWork, s32 unlockCount)
{
	throw __FUNCTION__;
}

s32 sceKernelGetLwMutexInfo(vm::psv::ptr<SceKernelLwMutexWork> pWork, vm::psv::ptr<SceKernelLwMutexInfo> pInfo)
{
	throw __FUNCTION__;
}

s32 sceKernelGetLwMutexInfoById(s32 lwMutexId, vm::psv::ptr<SceKernelLwMutexInfo> pInfo)
{
	throw __FUNCTION__;
}

// Condition variable functions

s32 sceKernelCreateCond(vm::psv::ptr<const char> pName, u32 attr, s32 mutexId, vm::psv::ptr<const SceKernelCondOptParam> pOptParam)
{
	sceLibKernel.Error("sceKernelCreateCond(pName=*0x%x, attr=0x%x, mutexId=0x%x, pOptParam=*0x%x)", pName, attr, mutexId, pOptParam);

	if (s32 id = g_psv_cond_list.add(new psv_cond_t(pName.get_ptr(), attr, mutexId), 0))
	{
		return id;
	}

	RETURN_ERROR(SCE_KERNEL_ERROR_ERROR);
}

s32 sceKernelDeleteCond(s32 condId)
{
	throw __FUNCTION__;
}

s32 sceKernelOpenCond(vm::psv::ptr<const char> pName)
{
	throw __FUNCTION__;
}

s32 sceKernelCloseCond(s32 condId)
{
	throw __FUNCTION__;
}

s32 sceKernelWaitCond(s32 condId, vm::psv::ptr<u32> pTimeout)
{
	throw __FUNCTION__;
}

s32 sceKernelWaitCondCB(s32 condId, vm::psv::ptr<u32> pTimeout)
{
	throw __FUNCTION__;
}

s32 sceKernelSignalCond(s32 condId)
{
	throw __FUNCTION__;
}

s32 sceKernelSignalCondAll(s32 condId)
{
	throw __FUNCTION__;
}

s32 sceKernelSignalCondTo(s32 condId, s32 threadId)
{
	throw __FUNCTION__;
}

s32 sceKernelGetCondInfo(s32 condId, vm::psv::ptr<SceKernelCondInfo> pInfo)
{
	throw __FUNCTION__;
}

// Lightweight condition variable functions

s32 sceKernelCreateLwCond(vm::psv::ptr<SceKernelLwCondWork> pWork, vm::psv::ptr<const char> pName, u32 attr, vm::psv::ptr<SceKernelLwMutexWork> pLwMutex, vm::psv::ptr<const SceKernelLwCondOptParam> pOptParam)
{
	throw __FUNCTION__;
}

s32 sceKernelDeleteLwCond(vm::psv::ptr<SceKernelLwCondWork> pWork)
{
	throw __FUNCTION__;
}

s32 sceKernelWaitLwCond(vm::psv::ptr<SceKernelLwCondWork> pWork, vm::psv::ptr<u32> pTimeout)
{
	throw __FUNCTION__;
}

s32 sceKernelWaitLwCondCB(vm::psv::ptr<SceKernelLwCondWork> pWork, vm::psv::ptr<u32> pTimeout)
{
	throw __FUNCTION__;
}

s32 sceKernelSignalLwCond(vm::psv::ptr<SceKernelLwCondWork> pWork)
{
	throw __FUNCTION__;
}

s32 sceKernelSignalLwCondAll(vm::psv::ptr<SceKernelLwCondWork> pWork)
{
	throw __FUNCTION__;
}

s32 sceKernelSignalLwCondTo(vm::psv::ptr<SceKernelLwCondWork> pWork, s32 threadId)
{
	throw __FUNCTION__;
}

s32 sceKernelGetLwCondInfo(vm::psv::ptr<SceKernelLwCondWork> pWork, vm::psv::ptr<SceKernelLwCondInfo> pInfo)
{
	throw __FUNCTION__;
}

s32 sceKernelGetLwCondInfoById(s32 lwCondId, vm::psv::ptr<SceKernelLwCondInfo> pInfo)
{
	throw __FUNCTION__;
}

// Time functions

s32 sceKernelGetSystemTime(vm::psv::ptr<SceKernelSysClock> pClock)
{
	throw __FUNCTION__;
}

u64 sceKernelGetSystemTimeWide()
{
	throw __FUNCTION__;
}

u32 sceKernelGetSystemTimeLow()
{
	throw __FUNCTION__;
}

// Timer functions

s32 sceKernelCreateTimer(vm::psv::ptr<const char> pName, u32 attr, vm::psv::ptr<const SceKernelTimerOptParam> pOptParam)
{
	throw __FUNCTION__;
}

s32 sceKernelDeleteTimer(s32 timerId)
{
	throw __FUNCTION__;
}

s32 sceKernelOpenTimer(vm::psv::ptr<const char> pName)
{
	throw __FUNCTION__;
}

s32 sceKernelCloseTimer(s32 timerId)
{
	throw __FUNCTION__;
}

s32 sceKernelStartTimer(s32 timerId)
{
	throw __FUNCTION__;
}

s32 sceKernelStopTimer(s32 timerId)
{
	throw __FUNCTION__;
}

s32 sceKernelGetTimerBase(s32 timerId, vm::psv::ptr<SceKernelSysClock> pBase)
{
	throw __FUNCTION__;
}

u64 sceKernelGetTimerBaseWide(s32 timerId)
{
	throw __FUNCTION__;
}

s32 sceKernelGetTimerTime(s32 timerId, vm::psv::ptr<SceKernelSysClock> pClock)
{
	throw __FUNCTION__;
}

u64 sceKernelGetTimerTimeWide(s32 timerId)
{
	throw __FUNCTION__;
}

s32 sceKernelSetTimerTime(s32 timerId, vm::psv::ptr<SceKernelSysClock> pClock)
{
	throw __FUNCTION__;
}

u64 sceKernelSetTimerTimeWide(s32 timerId, u64 clock)
{
	throw __FUNCTION__;
}

s32 sceKernelSetTimerEvent(s32 timerId, s32 type, vm::psv::ptr<SceKernelSysClock> pInterval, s32 fRepeat)
{
	throw __FUNCTION__;
}

s32 sceKernelCancelTimer(s32 timerId, vm::psv::ptr<s32> pNumWaitThreads)
{
	throw __FUNCTION__;
}

s32 sceKernelGetTimerInfo(s32 timerId, vm::psv::ptr<SceKernelTimerInfo> pInfo)
{
	throw __FUNCTION__;
}

// Reader/writer lock functions

s32 sceKernelCreateRWLock(vm::psv::ptr<const char> pName, u32 attr, vm::psv::ptr<const SceKernelRWLockOptParam> pOptParam)
{
	throw __FUNCTION__;
}

s32 sceKernelDeleteRWLock(s32 rwLockId)
{
	throw __FUNCTION__;
}

s32 sceKernelOpenRWLock(vm::psv::ptr<const char> pName)
{
	throw __FUNCTION__;
}

s32 sceKernelCloseRWLock(s32 rwLockId)
{
	throw __FUNCTION__;
}

s32 sceKernelLockReadRWLock(s32 rwLockId, vm::psv::ptr<u32> pTimeout)
{
	throw __FUNCTION__;
}

s32 sceKernelLockReadRWLockCB(s32 rwLockId, vm::psv::ptr<u32> pTimeout)
{
	throw __FUNCTION__;
}

s32 sceKernelTryLockReadRWLock(s32 rwLockId)
{
	throw __FUNCTION__;
}

s32 sceKernelUnlockReadRWLock(s32 rwLockId)
{
	throw __FUNCTION__;
}

s32 sceKernelLockWriteRWLock(s32 rwLockId, vm::psv::ptr<u32> pTimeout)
{
	throw __FUNCTION__;
}

s32 sceKernelLockWriteRWLockCB(s32 rwLockId, vm::psv::ptr<u32> pTimeout)
{
	throw __FUNCTION__;
}

s32 sceKernelTryLockWriteRWLock(s32 rwLockId)
{
	throw __FUNCTION__;
}

s32 sceKernelUnlockWriteRWLock(s32 rwLockId)
{
	throw __FUNCTION__;
}

s32 sceKernelCancelRWLock(s32 rwLockId, vm::psv::ptr<s32> pNumReadWaitThreads, vm::psv::ptr<s32> pNumWriteWaitThreads, s32 flag)
{
	throw __FUNCTION__;
}

s32 sceKernelGetRWLockInfo(s32 rwLockId, vm::psv::ptr<SceKernelRWLockInfo> pInfo)
{
	throw __FUNCTION__;
}

// IO/File functions

s32 sceIoRemove(vm::psv::ptr<const char> filename)
{
	throw __FUNCTION__;
}

s32 sceIoMkdir(vm::psv::ptr<const char> dirname, s32 mode)
{
	throw __FUNCTION__;
}

s32 sceIoRmdir(vm::psv::ptr<const char> dirname)
{
	throw __FUNCTION__;
}

s32 sceIoRename(vm::psv::ptr<const char> oldname, vm::psv::ptr<const char> newname)
{
	throw __FUNCTION__;
}

s32 sceIoDevctl(vm::psv::ptr<const char> devname, s32 cmd, vm::psv::ptr<const void> arg, u32 arglen, vm::psv::ptr<void> bufp, u32 buflen)
{
	throw __FUNCTION__;
}

s32 sceIoSync(vm::psv::ptr<const char> devname, s32 flag)
{
	throw __FUNCTION__;
}

s32 sceIoOpen(vm::psv::ptr<const char> filename, s32 flag, s32 mode)
{
	throw __FUNCTION__;
}

s32 sceIoClose(s32 fd)
{
	throw __FUNCTION__;
}

s32 sceIoIoctl(s32 fd, s32 cmd, vm::psv::ptr<const void> argp, u32 arglen, vm::psv::ptr<void> bufp, u32 buflen)
{
	throw __FUNCTION__;
}

s64 sceIoLseek(s32 fd, s64 offset, s32 whence)
{
	throw __FUNCTION__;
}

s32 sceIoLseek32(s32 fd, s32 offset, s32 whence)
{
	throw __FUNCTION__;
}

s32 sceIoRead(s32 fd, vm::psv::ptr<void> buf, u32 nbyte)
{
	throw __FUNCTION__;
}

s32 sceIoWrite(s32 fd, vm::psv::ptr<const void> buf, u32 nbyte)
{
	throw __FUNCTION__;
}

s32 sceIoPread(s32 fd, vm::psv::ptr<void> buf, u32 nbyte, s64 offset)
{
	throw __FUNCTION__;
}

s32 sceIoPwrite(s32 fd, vm::psv::ptr<const void> buf, u32 nbyte, s64 offset)
{
	throw __FUNCTION__;
}

s32 sceIoDopen(vm::psv::ptr<const char> dirname)
{
	throw __FUNCTION__;
}

s32 sceIoDclose(s32 fd)
{
	throw __FUNCTION__;
}

s32 sceIoDread(s32 fd, vm::psv::ptr<SceIoDirent> buf)
{
	throw __FUNCTION__;
}

s32 sceIoChstat(vm::psv::ptr<const char> name, vm::psv::ptr<const SceIoStat> buf, u32 cbit)
{
	throw __FUNCTION__;
}

s32 sceIoGetstat(vm::psv::ptr<const char> name, vm::psv::ptr<SceIoStat> buf)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceLibKernel, #name, name)

psv_log_base sceLibKernel("sceLibKernel", []()
{
	sceLibKernel.on_load = nullptr;
	sceLibKernel.on_unload = nullptr;
	sceLibKernel.on_stop = nullptr;

	// REG_FUNC(???, sceKernelGetEventInfo);

	//REG_FUNC(0x023EAA62, sceKernelPuts);
	//REG_FUNC(0xB0335388, sceClibToupper);
	//REG_FUNC(0x4C5471BC, sceClibTolower);
	//REG_FUNC(0xD8EBBB7E, sceClibLookCtypeTable);
	//REG_FUNC(0x20EC3210, sceClibGetCtypeTable);
	//REG_FUNC(0x407D6153, sceClibMemchr);
	//REG_FUNC(0x9CC2BFDF, sceClibMemcmp);
	//REG_FUNC(0x14E9DBD7, sceClibMemcpy);
	//REG_FUNC(0x736753C8, sceClibMemmove);
	//REG_FUNC(0x632980D7, sceClibMemset);
	//REG_FUNC(0xFA26BC62, sceClibPrintf);
	//REG_FUNC(0x5EA3B6CE, sceClibVprintf);
	//REG_FUNC(0x8CBA03D5, sceClibSnprintf);
	//REG_FUNC(0xFA6BE467, sceClibVsnprintf);
	//REG_FUNC(0xA2FB4D9D, sceClibStrcmp);
	//REG_FUNC(0x70CBC2D5, sceClibStrlcat);
	//REG_FUNC(0x2CDFCD1C, sceClibStrlcpy);
	//REG_FUNC(0xA37E6383, sceClibStrncat);
	//REG_FUNC(0x660D1F6D, sceClibStrncmp);
	//REG_FUNC(0xC458D60A, sceClibStrncpy);
	//REG_FUNC(0xAC595E68, sceClibStrnlen);
	//REG_FUNC(0x614076B7, sceClibStrchr);
	//REG_FUNC(0x6E728AAE, sceClibStrrchr);
	//REG_FUNC(0xE265498B, sceClibStrstr);
	//REG_FUNC(0xB54C0BE4, sceClibStrncasecmp);
	//REG_FUNC(0x2F2C6046, sceClibAbort);
	//REG_FUNC(0x2E581B88, sceClibStrtoll);
	//REG_FUNC(0x3B9E301A, sceClibMspaceCreate);
	//REG_FUNC(0xAE1A21EC, sceClibMspaceDestroy);
	//REG_FUNC(0x86EF7680, sceClibMspaceMalloc);
	//REG_FUNC(0x9C56B4D1, sceClibMspaceFree);
	//REG_FUNC(0x678374AD, sceClibMspaceCalloc);
	//REG_FUNC(0x3C847D57, sceClibMspaceMemalign);
	//REG_FUNC(0x774891D6, sceClibMspaceRealloc);
	//REG_FUNC(0x586AC68D, sceClibMspaceReallocalign);
	//REG_FUNC(0x46A02279, sceClibMspaceMallocUsableSize);
	//REG_FUNC(0x8CC1D38E, sceClibMspaceMallocStats);
	//REG_FUNC(0x738E0322, sceClibMspaceMallocStatsFast);
	//REG_FUNC(0xD1D59701, sceClibMspaceIsHeapEmpty);
	//REG_FUNC(0xE960FDA2, sceKernelAtomicSet8);
	//REG_FUNC(0x450BFECF, sceKernelAtomicSet16);
	//REG_FUNC(0xB69DA09B, sceKernelAtomicSet32);
	//REG_FUNC(0xC8A4339C, sceKernelAtomicSet64);
	//REG_FUNC(0x27A2AAFA, sceKernelAtomicGetAndAdd8);
	//REG_FUNC(0x5674DB0C, sceKernelAtomicGetAndAdd16);
	//REG_FUNC(0x2611CB0B, sceKernelAtomicGetAndAdd32);
	//REG_FUNC(0x63DAF37D, sceKernelAtomicGetAndAdd64);
	//REG_FUNC(0x8F7BD940, sceKernelAtomicAddAndGet8);
	//REG_FUNC(0x495C52EC, sceKernelAtomicAddAndGet16);
	//REG_FUNC(0x2E84A93B, sceKernelAtomicAddAndGet32);
	//REG_FUNC(0xB6CE9B9A, sceKernelAtomicAddAndGet64);
	//REG_FUNC(0xCDF5DF67, sceKernelAtomicGetAndSub8);
	//REG_FUNC(0xAC51979C, sceKernelAtomicGetAndSub16);
	//REG_FUNC(0x115C516F, sceKernelAtomicGetAndSub32);
	//REG_FUNC(0x4AE9C8E6, sceKernelAtomicGetAndSub64);
	//REG_FUNC(0x99E1796E, sceKernelAtomicSubAndGet8);
	//REG_FUNC(0xC26BBBB1, sceKernelAtomicSubAndGet16);
	//REG_FUNC(0x01C9CD92, sceKernelAtomicSubAndGet32);
	//REG_FUNC(0x9BB4A94B, sceKernelAtomicSubAndGet64);
	//REG_FUNC(0x53DCA02B, sceKernelAtomicGetAndAnd8);
	//REG_FUNC(0x7A0CB056, sceKernelAtomicGetAndAnd16);
	//REG_FUNC(0x08266595, sceKernelAtomicGetAndAnd32);
	//REG_FUNC(0x4828BC43, sceKernelAtomicGetAndAnd64);
	//REG_FUNC(0x86B9170F, sceKernelAtomicAndAndGet8);
	//REG_FUNC(0xF9890F7E, sceKernelAtomicAndAndGet16);
	//REG_FUNC(0x6709D30C, sceKernelAtomicAndAndGet32);
	//REG_FUNC(0xAED2B370, sceKernelAtomicAndAndGet64);
	//REG_FUNC(0x107A68DF, sceKernelAtomicGetAndOr8);
	//REG_FUNC(0x31E49E73, sceKernelAtomicGetAndOr16);
	//REG_FUNC(0x984AD276, sceKernelAtomicGetAndOr32);
	//REG_FUNC(0xC39186CD, sceKernelAtomicGetAndOr64);
	//REG_FUNC(0x51693931, sceKernelAtomicOrAndGet8);
	//REG_FUNC(0x8E248EBD, sceKernelAtomicOrAndGet16);
	//REG_FUNC(0xC3B2F7F8, sceKernelAtomicOrAndGet32);
	//REG_FUNC(0x809BBC7D, sceKernelAtomicOrAndGet64);
	//REG_FUNC(0x7350B2DF, sceKernelAtomicGetAndXor8);
	//REG_FUNC(0x6E2D0B9E, sceKernelAtomicGetAndXor16);
	//REG_FUNC(0x38739E2F, sceKernelAtomicGetAndXor32);
	//REG_FUNC(0x6A19BBE9, sceKernelAtomicGetAndXor64);
	//REG_FUNC(0x634AF062, sceKernelAtomicXorAndGet8);
	//REG_FUNC(0x6F524195, sceKernelAtomicXorAndGet16);
	//REG_FUNC(0x46940704, sceKernelAtomicXorAndGet32);
	//REG_FUNC(0xDDC6866E, sceKernelAtomicXorAndGet64);
	//REG_FUNC(0x327DB4C0, sceKernelAtomicCompareAndSet8);
	//REG_FUNC(0xE8C01236, sceKernelAtomicCompareAndSet16);
	//REG_FUNC(0x1124A1D4, sceKernelAtomicCompareAndSet32);
	//REG_FUNC(0x1EBDFCCD, sceKernelAtomicCompareAndSet64);
	//REG_FUNC(0xD7D49E36, sceKernelAtomicClearMask8);
	//REG_FUNC(0x5FE7DFF8, sceKernelAtomicClearMask16);
	//REG_FUNC(0xE3DF0CB3, sceKernelAtomicClearMask32);
	//REG_FUNC(0x953D118A, sceKernelAtomicClearMask64);
	//REG_FUNC(0x7FD94393, sceKernelAtomicAddUnless8);
	//REG_FUNC(0x1CF4AA4B, sceKernelAtomicAddUnless16);
	//REG_FUNC(0x4B33FD3C, sceKernelAtomicAddUnless32);
	//REG_FUNC(0xFFCE7438, sceKernelAtomicAddUnless64);
	//REG_FUNC(0x9DABE6C3, sceKernelAtomicDecIfPositive8);
	//REG_FUNC(0x323718FB, sceKernelAtomicDecIfPositive16);
	//REG_FUNC(0xCA3294F1, sceKernelAtomicDecIfPositive32);
	//REG_FUNC(0x8BE2A007, sceKernelAtomicDecIfPositive64);
	//REG_FUNC(0xBBE82155, sceKernelLoadModule);
	//REG_FUNC(0x2DCC4AFA, sceKernelLoadStartModule);
	//REG_FUNC(0x702425D5, sceKernelStartModule);
	//REG_FUNC(0x3B2CBA09, sceKernelStopModule);
	//REG_FUNC(0x1987920E, sceKernelUnloadModule);
	//REG_FUNC(0x2415F8A4, sceKernelStopUnloadModule);
	//REG_FUNC(0x15E2A45D, sceKernelCallModuleExit);
	//REG_FUNC(0xD11A5103, sceKernelGetModuleInfoByAddr);
	//REG_FUNC(0x4F2D8B15, sceKernelOpenModule);
	//REG_FUNC(0x657FA50E, sceKernelCloseModule);
	//REG_FUNC(0x7595D9AA, sceKernelExitProcess);
	//REG_FUNC(0x4C7AD128, sceKernelPowerLock);
	//REG_FUNC(0xAF8E9C11, sceKernelPowerUnlock);
	//REG_FUNC(0xB295EB61, sceKernelGetTLSAddr);
	REG_FUNC(0x0FB972F9, sceKernelGetThreadId);
	REG_FUNC(0xA37A6057, sceKernelGetCurrentThreadVfpException);
	//REG_FUNC(0x0CA71EA2, sceKernelSendMsgPipe);
	//REG_FUNC(0xA5CA74AC, sceKernelSendMsgPipeCB);
	//REG_FUNC(0xDFC670E0, sceKernelTrySendMsgPipe);
	//REG_FUNC(0x4E81DD5C, sceKernelReceiveMsgPipe);
	//REG_FUNC(0x33AF829B, sceKernelReceiveMsgPipeCB);
	//REG_FUNC(0x5615B006, sceKernelTryReceiveMsgPipe);
	REG_FUNC(0xA7819967, sceKernelLockLwMutex);
	REG_FUNC(0x6F9C4CC1, sceKernelLockLwMutexCB);
	REG_FUNC(0x9EF798C1, sceKernelTryLockLwMutex);
	REG_FUNC(0x499EA781, sceKernelUnlockLwMutex);
	REG_FUNC(0xF7D8F1FC, sceKernelGetLwMutexInfo);
	REG_FUNC(0xDDB395A9, sceKernelWaitThreadEnd);
	REG_FUNC(0xC54941ED, sceKernelWaitThreadEndCB);
	REG_FUNC(0xD5DC26C4, sceKernelGetThreadExitStatus);
	//REG_FUNC(0x4373B548, __sce_aeabi_idiv0);
	//REG_FUNC(0xFB235848, __sce_aeabi_ldiv0);
	REG_FUNC(0xF08DE149, sceKernelStartThread);
	REG_FUNC(0x58DDAC4F, sceKernelDeleteThread);
	REG_FUNC(0x5150577B, sceKernelChangeThreadCpuAffinityMask);
	REG_FUNC(0x8C57AC2A, sceKernelGetThreadCpuAffinityMask);
	REG_FUNC(0xDF7E6EDA, sceKernelChangeThreadPriority);
	//REG_FUNC(0xBCB63B66, sceKernelGetThreadStackFreeSize);
	REG_FUNC(0x8D9C5461, sceKernelGetThreadInfo);
	REG_FUNC(0xD6B01013, sceKernelGetThreadRunStatus);
	REG_FUNC(0xE0241FAA, sceKernelGetSystemInfo);
	REG_FUNC(0xF994FE65, sceKernelGetThreadmgrUIDClass);
	//REG_FUNC(0xB4DE10C7, sceKernelGetActiveCpuMask);
	REG_FUNC(0x2C1321A3, sceKernelChangeThreadVfpException);
	REG_FUNC(0x3849359A, sceKernelCreateCallback);
	REG_FUNC(0x88DD1BC8, sceKernelGetCallbackInfo);
	REG_FUNC(0x464559D3, sceKernelDeleteCallback);
	REG_FUNC(0xBD9C8F2B, sceKernelNotifyCallback);
	REG_FUNC(0x3137A687, sceKernelCancelCallback);
	REG_FUNC(0x76A2EF81, sceKernelGetCallbackCount);
	REG_FUNC(0xD4F75281, sceKernelRegisterCallbackToEvent);
	REG_FUNC(0x8D3940DF, sceKernelUnregisterCallbackFromEvent);
	REG_FUNC(0x2BD1E682, sceKernelUnregisterCallbackFromEventAll);
	REG_FUNC(0x120F03AF, sceKernelWaitEvent);
	REG_FUNC(0xA0490795, sceKernelWaitEventCB);
	REG_FUNC(0x241F3634, sceKernelPollEvent);
	REG_FUNC(0x603AB770, sceKernelCancelEvent);
	REG_FUNC(0x10586418, sceKernelWaitMultipleEvents);
	REG_FUNC(0x4263DBC9, sceKernelWaitMultipleEventsCB);
	REG_FUNC(0x8516D040, sceKernelCreateEventFlag);
	REG_FUNC(0x11FE9B8B, sceKernelDeleteEventFlag);
	REG_FUNC(0xE04EC73A, sceKernelOpenEventFlag);
	REG_FUNC(0x9C0B8285, sceKernelCloseEventFlag);
	REG_FUNC(0x83C0E2AF, sceKernelWaitEventFlag);
	REG_FUNC(0xE737B1DF, sceKernelWaitEventFlagCB);
	REG_FUNC(0x1FBB0FE1, sceKernelPollEventFlag);
	REG_FUNC(0x2A12D9B7, sceKernelCancelEventFlag);
	REG_FUNC(0x8BA4C0C1, sceKernelGetEventFlagInfo);
	REG_FUNC(0x9EF9C0C5, sceKernelSetEventFlag);
	REG_FUNC(0xD018793F, sceKernelClearEventFlag);
	REG_FUNC(0x297AA2AE, sceKernelCreateSema);
	REG_FUNC(0xC08F5BC5, sceKernelDeleteSema);
	REG_FUNC(0xB028AB78, sceKernelOpenSema);
	REG_FUNC(0x817707AB, sceKernelCloseSema);
	REG_FUNC(0x0C7B834B, sceKernelWaitSema);
	REG_FUNC(0x174692B4, sceKernelWaitSemaCB);
	REG_FUNC(0x66D6BF05, sceKernelCancelSema);
	REG_FUNC(0x595D3FA6, sceKernelGetSemaInfo);
	REG_FUNC(0x3012A9C6, sceKernelPollSema);
	REG_FUNC(0x2053A496, sceKernelSignalSema);
	REG_FUNC(0xED53334A, sceKernelCreateMutex);
	REG_FUNC(0x12D11F65, sceKernelDeleteMutex);
	REG_FUNC(0x16B85235, sceKernelOpenMutex);
	REG_FUNC(0x43DDC9CC, sceKernelCloseMutex);
	REG_FUNC(0x1D8D7945, sceKernelLockMutex);
	REG_FUNC(0x2BDAA524, sceKernelLockMutexCB);
	REG_FUNC(0x2144890D, sceKernelCancelMutex);
	REG_FUNC(0x9A6C43CA, sceKernelGetMutexInfo);
	REG_FUNC(0xE5901FF9, sceKernelTryLockMutex);
	REG_FUNC(0x34746309, sceKernelUnlockMutex);
	REG_FUNC(0x50572FDA, sceKernelCreateCond);
	REG_FUNC(0xFD295414, sceKernelDeleteCond);
	REG_FUNC(0xCB2A73A9, sceKernelOpenCond);
	REG_FUNC(0x4FB91A89, sceKernelCloseCond);
	REG_FUNC(0xC88D44AD, sceKernelWaitCond);
	REG_FUNC(0x4CE42CE2, sceKernelWaitCondCB);
	REG_FUNC(0x6864DCE2, sceKernelGetCondInfo);
	REG_FUNC(0x10A4976F, sceKernelSignalCond);
	REG_FUNC(0x2EB86929, sceKernelSignalCondAll);
	REG_FUNC(0x087629E6, sceKernelSignalCondTo);
	//REG_FUNC(0x0A10C1C8, sceKernelCreateMsgPipe);
	//REG_FUNC(0x69F6575D, sceKernelDeleteMsgPipe);
	//REG_FUNC(0x230691DA, sceKernelOpenMsgPipe);
	//REG_FUNC(0x7E5C0C16, sceKernelCloseMsgPipe);
	//REG_FUNC(0x94D506F7, sceKernelSendMsgPipeVector);
	//REG_FUNC(0x9C6F7F79, sceKernelSendMsgPipeVectorCB);
	//REG_FUNC(0x60DB346F, sceKernelTrySendMsgPipeVector);
	//REG_FUNC(0x9F899087, sceKernelReceiveMsgPipeVector);
	//REG_FUNC(0xBE5B3E27, sceKernelReceiveMsgPipeVectorCB);
	//REG_FUNC(0x86ECC0FF, sceKernelTryReceiveMsgPipeVector);
	//REG_FUNC(0xEF14BA37, sceKernelCancelMsgPipe);
	//REG_FUNC(0x4046D16B, sceKernelGetMsgPipeInfo);
	REG_FUNC(0xDA6EC8EF, sceKernelCreateLwMutex);
	REG_FUNC(0x244E76D2, sceKernelDeleteLwMutex);
	REG_FUNC(0x4846613D, sceKernelGetLwMutexInfoById);
	REG_FUNC(0x48C7EAE6, sceKernelCreateLwCond);
	REG_FUNC(0x721F6CB3, sceKernelDeleteLwCond);
	REG_FUNC(0xE1878282, sceKernelWaitLwCond);
	REG_FUNC(0x8FA54B07, sceKernelWaitLwCondCB);
	REG_FUNC(0x3AC63B9A, sceKernelSignalLwCond);
	REG_FUNC(0xE5241A0C, sceKernelSignalLwCondAll);
	REG_FUNC(0xFC1A48EB, sceKernelSignalLwCondTo);
	REG_FUNC(0xE4DF36A0, sceKernelGetLwCondInfo);
	REG_FUNC(0x971F1DE8, sceKernelGetLwCondInfoById);
	REG_FUNC(0x2255B2A5, sceKernelCreateTimer);
	REG_FUNC(0x746F3290, sceKernelDeleteTimer);
	REG_FUNC(0x2F3D35A3, sceKernelOpenTimer);
	REG_FUNC(0x17283DE6, sceKernelCloseTimer);
	REG_FUNC(0x1478249B, sceKernelStartTimer);
	REG_FUNC(0x075B1329, sceKernelStopTimer);
	REG_FUNC(0x1F59E04D, sceKernelGetTimerBase);
	REG_FUNC(0x3223CCD1, sceKernelGetTimerBaseWide);
	REG_FUNC(0x381DC300, sceKernelGetTimerTime);
	REG_FUNC(0x53C5D833, sceKernelGetTimerTimeWide);
	REG_FUNC(0xFFAD717F, sceKernelSetTimerTime);
	REG_FUNC(0xAF67678B, sceKernelSetTimerTimeWide);
	REG_FUNC(0x621D293B, sceKernelSetTimerEvent);
	REG_FUNC(0x9CCF768C, sceKernelCancelTimer);
	REG_FUNC(0x7E35E10A, sceKernelGetTimerInfo);
	REG_FUNC(0x8667951D, sceKernelCreateRWLock);
	REG_FUNC(0x3D750204, sceKernelDeleteRWLock);
	REG_FUNC(0xBA4DAC9A, sceKernelOpenRWLock);
	REG_FUNC(0xA7F94E64, sceKernelCloseRWLock);
	REG_FUNC(0xFA670F0F, sceKernelLockReadRWLock);
	REG_FUNC(0x2D4A62B7, sceKernelLockReadRWLockCB);
	REG_FUNC(0x1B8586C0, sceKernelTryLockReadRWLock);
	REG_FUNC(0x675D10A8, sceKernelUnlockReadRWLock);
	REG_FUNC(0x67A187BB, sceKernelLockWriteRWLock);
	REG_FUNC(0xA4777082, sceKernelLockWriteRWLockCB);
	REG_FUNC(0x597D4607, sceKernelTryLockWriteRWLock);
	REG_FUNC(0xD9369DF2, sceKernelUnlockWriteRWLock);
	REG_FUNC(0x190CA94B, sceKernelCancelRWLock);
	REG_FUNC(0x079A573B, sceKernelGetRWLockInfo);
	REG_FUNC(0x8AF15B5F, sceKernelGetSystemTime);
	//REG_FUNC(0x99B2BF15, sceKernelPMonThreadGetCounter);
	//REG_FUNC(0x7C21C961, sceKernelPMonCpuGetCounter);
	//REG_FUNC(0xADCA94E5, sceKernelWaitSignal);
	//REG_FUNC(0x24460BB3, sceKernelWaitSignalCB);
	//REG_FUNC(0x7BE9C4C8, sceKernelSendSignal);
	REG_FUNC(0xC5C11EE7, sceKernelCreateThread);
	REG_FUNC(0x6C60AC61, sceIoOpen);
	REG_FUNC(0xF5C6F098, sceIoClose);
	REG_FUNC(0x713523E1, sceIoRead);
	REG_FUNC(0x11FED231, sceIoWrite);
	REG_FUNC(0x99BA173E, sceIoLseek);
	REG_FUNC(0x5CC983AC, sceIoLseek32);
	REG_FUNC(0xE20ED0F3, sceIoRemove);
	REG_FUNC(0xF737E369, sceIoRename);
	REG_FUNC(0x9670D39F, sceIoMkdir);
	REG_FUNC(0xE9F91EC8, sceIoRmdir);
	REG_FUNC(0xA9283DD0, sceIoDopen);
	REG_FUNC(0x9DFF9C59, sceIoDclose);
	REG_FUNC(0x9C8B6624, sceIoDread);
	REG_FUNC(0xBCA5B623, sceIoGetstat);
	REG_FUNC(0x29482F7F, sceIoChstat);
	REG_FUNC(0x98ACED6D, sceIoSync);
	REG_FUNC(0x04B30CB2, sceIoDevctl);
	REG_FUNC(0x54ABACFA, sceIoIoctl);
	//REG_FUNC(0x6A7EA9FD, sceIoOpenAsync);
	//REG_FUNC(0x84201C9B, sceIoCloseAsync);
	//REG_FUNC(0x7B3BE857, sceIoReadAsync);
	//REG_FUNC(0x21329B20, sceIoWriteAsync);
	//REG_FUNC(0xCAC5D672, sceIoLseekAsync);
	//REG_FUNC(0x099C54B9, sceIoIoctlAsync);
	//REG_FUNC(0x446A60AC, sceIoRemoveAsync);
	//REG_FUNC(0x73FC184B, sceIoDopenAsync);
	//REG_FUNC(0x4D0597D7, sceIoDcloseAsync);
	//REG_FUNC(0xCE32490D, sceIoDreadAsync);
	//REG_FUNC(0x8E5FCBB1, sceIoMkdirAsync);
	//REG_FUNC(0x9694D00F, sceIoRmdirAsync);
	//REG_FUNC(0xEE9857CD, sceIoRenameAsync);
	//REG_FUNC(0x9739A5E2, sceIoChstatAsync);
	//REG_FUNC(0x82B20B41, sceIoGetstatAsync);
	//REG_FUNC(0x950F78EB, sceIoDevctlAsync);
	//REG_FUNC(0xF7C7FBFE, sceIoSyncAsync);
	//REG_FUNC(0xEC96EA71, sceIoCancel);
	//REG_FUNC(0x857E0C71, sceIoComplete);
	REG_FUNC(0x52315AD7, sceIoPread);
	REG_FUNC(0x8FFFF5A8, sceIoPwrite);
	//REG_FUNC(0xA010141E, sceIoPreadAsync);
	//REG_FUNC(0xED25BEEF, sceIoPwriteAsync);
	//REG_FUNC(0xA792C404, sceIoCompleteMultiple);
	//REG_FUNC(0x894037E8, sceKernelBacktrace);
	//REG_FUNC(0x20E2D4B7, sceKernelPrintBacktrace);
	//REG_FUNC(0x963F4A99, sceSblACMgrIsGameProgram);
	//REG_FUNC(0x261E2C34, sceKernelGetOpenPsId);

	/* SceModulemgr */
	//REG_FUNC(0x36585DAF, sceKernelGetModuleInfo);
	//REG_FUNC(0x2EF2581F, sceKernelGetModuleList);
	//REG_FUNC(0xF5798C7C, sceKernelGetModuleIdByAddr);

	/* SceProcessmgr */
	//REG_FUNC(0xCD248267, sceKernelGetCurrentProcess);
	//REG_FUNC(0x2252890C, sceKernelPowerTick);
	//REG_FUNC(0x9E45DA09, sceKernelLibcClock);
	//REG_FUNC(0x0039BE45, sceKernelLibcTime);
	//REG_FUNC(0x4B879059, sceKernelLibcGettimeofday);
	//REG_FUNC(0xC1727F59, sceKernelGetStdin);
	//REG_FUNC(0xE5AA625C, sceKernelGetStdout);
	//REG_FUNC(0xFA5E3ADA, sceKernelGetStderr);
	//REG_FUNC(0xE6E9FCA3, sceKernelGetRemoteProcessTime);
	//REG_FUNC(0xD37A8437, sceKernelGetProcessTime);
	//REG_FUNC(0xF5D0D4C6, sceKernelGetProcessTimeLow);
	//REG_FUNC(0x89DA0967, sceKernelGetProcessTimeWide);
	//REG_FUNC(0x2BE3E066, sceKernelGetProcessParam);

	/* SceStdio */
	//REG_FUNC(0x54237407, sceKernelStdin);
	//REG_FUNC(0x9033E9BD, sceKernelStdout);
	//REG_FUNC(0x35EE7CF5, sceKernelStderr);

	/* SceSysmem */
	REG_FUNC(0xB9D5EBDE, sceKernelAllocMemBlock);
	REG_FUNC(0xA91E15EE, sceKernelFreeMemBlock);
	REG_FUNC(0xB8EF5818, sceKernelGetMemBlockBase);
	//REG_FUNC(0x3B29E0F5, sceKernelRemapMemBlock);
	//REG_FUNC(0xA33B99D1, sceKernelFindMemBlockByAddr);
	REG_FUNC(0x4010AD65, sceKernelGetMemBlockInfoByAddr);

	/* SceCpu */
	//REG_FUNC(0x2704CFEE, sceKernelCpuId);

	/* SceDipsw */
	//REG_FUNC(0x1C783FB2, sceKernelCheckDipsw);
	//REG_FUNC(0x817053D4, sceKernelSetDipsw);
	//REG_FUNC(0x800EDCC1, sceKernelClearDipsw);

	/* SceThreadmgr */
	REG_FUNC(0x0C8A38E1, sceKernelExitThread);
	REG_FUNC(0x1D17DECF, sceKernelExitDeleteThread);
	REG_FUNC(0x4B675D05, sceKernelDelayThread);
	REG_FUNC(0x9C0180E1, sceKernelDelayThreadCB);
	//REG_FUNC(0x001173F8, sceKernelChangeActiveCpuMask);
	REG_FUNC(0x01414F0B, sceKernelGetThreadCurrentPriority);
	REG_FUNC(0x751C9B7A, sceKernelChangeCurrentThreadAttr);
	REG_FUNC(0xD9BD74EB, sceKernelCheckWaitableStatus);
	REG_FUNC(0x9DCB4B7A, sceKernelGetProcessId);
	REG_FUNC(0xE53E41F6, sceKernelCheckCallback);
	REG_FUNC(0xF4EE4FA9, sceKernelGetSystemTimeWide);
	REG_FUNC(0x47F6DE49, sceKernelGetSystemTimeLow);
	//REG_FUNC(0xC0FAF6A3, sceKernelCreateThreadForUser);

	/* SceDebugLed */
	//REG_FUNC(0x78E702D3, sceKernelSetGPO);
});
