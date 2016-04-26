#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/ARMv7/ARMv7Module.h"

#include "sceLibKernel.h"

#include "Utilities/StrUtil.h"

LOG_CHANNEL(sceLibKernel);

extern u64 get_system_time();

extern std::condition_variable& get_current_thread_cv();

s32 sceKernelAllocMemBlock(vm::cptr<char> name, s32 type, u32 vsize, vm::ptr<SceKernelAllocMemBlockOpt> pOpt)
{
	throw EXCEPTION("");
}

s32 sceKernelFreeMemBlock(s32 uid)
{
	throw EXCEPTION("");
}

s32 sceKernelGetMemBlockBase(s32 uid, vm::pptr<void> ppBase)
{
	throw EXCEPTION("");
}

s32 sceKernelGetMemBlockInfoByAddr(vm::ptr<void> vbase, vm::ptr<SceKernelMemBlockInfo> pInfo)
{
	throw EXCEPTION("");
}

arm_error_code sceKernelCreateThread(vm::cptr<char> pName, vm::ptr<SceKernelThreadEntry> entry, s32 initPriority, u32 stackSize, u32 attr, s32 cpuAffinityMask, vm::cptr<SceKernelThreadOptParam> pOptParam)
{
	sceLibKernel.warning("sceKernelCreateThread(pName=*0x%x, entry=*0x%x, initPriority=%d, stackSize=0x%x, attr=0x%x, cpuAffinityMask=0x%x, pOptParam=*0x%x)",
		pName, entry, initPriority, stackSize, attr, cpuAffinityMask, pOptParam);

	const auto thread = idm::make_ptr<ARMv7Thread>(pName.get_ptr());

	thread->PC = entry.addr();
	thread->prio = initPriority;
	thread->stack_size = stackSize;
	thread->cpu_init();

	return NOT_AN_ERROR(thread->id);
}

arm_error_code sceKernelStartThread(s32 threadId, u32 argSize, vm::cptr<void> pArgBlock)
{
	sceLibKernel.warning("sceKernelStartThread(threadId=0x%x, argSize=0x%x, pArgBlock=*0x%x)", threadId, argSize, pArgBlock);

	const auto thread = idm::get<ARMv7Thread>(threadId);

	if (!thread)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	// thread should be in DORMANT state, but it's not possible to check it correctly atm

	//if (thread->IsAlive())
	//{
	//	return SCE_KERNEL_ERROR_NOT_DORMANT;
	//}

	// push arg block onto the stack
	const u32 pos = (thread->SP -= argSize);
	std::memcpy(vm::base(pos), pArgBlock.get_ptr(), argSize);

	// set SceKernelThreadEntry function arguments
	thread->GPR[0] = argSize;
	thread->GPR[1] = pos;

	thread->state -= cpu_state::stop;
	thread->lock_notify();
	return SCE_OK;
}

arm_error_code sceKernelExitThread(ARMv7Thread& cpu, s32 exitStatus)
{
	sceLibKernel.warning("sceKernelExitThread(exitStatus=0x%x)", exitStatus);

	// Exit status is stored in r0
	cpu.state += cpu_state::exit;

	return SCE_OK;
}

arm_error_code sceKernelDeleteThread(s32 threadId)
{
	sceLibKernel.warning("sceKernelDeleteThread(threadId=0x%x)", threadId);

	const auto thread = idm::get<ARMv7Thread>(threadId);

	if (!thread)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	// thread should be in DORMANT state, but it's not possible to check it correctly atm

	//if (thread->IsAlive())
	//{
	//	return SCE_KERNEL_ERROR_NOT_DORMANT;
	//}

	idm::remove<ARMv7Thread>(threadId);
	return SCE_OK;
}

arm_error_code sceKernelExitDeleteThread(ARMv7Thread& cpu, s32 exitStatus)
{
	sceLibKernel.warning("sceKernelExitDeleteThread(exitStatus=0x%x)", exitStatus);

	//cpu.state += cpu_state::stop;

	// Delete current thread; exit status is stored in r0
	idm::remove<ARMv7Thread>(cpu.id);

	return SCE_OK;
}

s32 sceKernelChangeThreadCpuAffinityMask(s32 threadId, s32 cpuAffinityMask)
{
	sceLibKernel.todo("sceKernelChangeThreadCpuAffinityMask(threadId=0x%x, cpuAffinityMask=0x%x)", threadId, cpuAffinityMask);

	throw EXCEPTION("");
}

s32 sceKernelGetThreadCpuAffinityMask(s32 threadId)
{
	sceLibKernel.todo("sceKernelGetThreadCpuAffinityMask(threadId=0x%x)", threadId);

	throw EXCEPTION("");
}

s32 sceKernelChangeThreadPriority(s32 threadId, s32 priority)
{
	sceLibKernel.todo("sceKernelChangeThreadPriority(threadId=0x%x, priority=%d)", threadId, priority);

	throw EXCEPTION("");
}

s32 sceKernelGetThreadCurrentPriority()
{
	sceLibKernel.todo("sceKernelGetThreadCurrentPriority()");

	throw EXCEPTION("");
}

u32 sceKernelGetThreadId(ARMv7Thread& cpu)
{
	sceLibKernel.trace("sceKernelGetThreadId()");

	return cpu.id;
}

s32 sceKernelChangeCurrentThreadAttr(u32 clearAttr, u32 setAttr)
{
	sceLibKernel.todo("sceKernelChangeCurrentThreadAttr()");

	throw EXCEPTION("");
}

s32 sceKernelGetThreadExitStatus(s32 threadId, vm::ptr<s32> pExitStatus)
{
	sceLibKernel.todo("sceKernelGetThreadExitStatus(threadId=0x%x, pExitStatus=*0x%x)", threadId, pExitStatus);

	throw EXCEPTION("");
}

s32 sceKernelGetProcessId()
{
	sceLibKernel.todo("sceKernelGetProcessId()");

	throw EXCEPTION("");
}

s32 sceKernelCheckWaitableStatus()
{
	sceLibKernel.todo("sceKernelCheckWaitableStatus()");

	throw EXCEPTION("");
}

s32 sceKernelGetThreadInfo(s32 threadId, vm::ptr<SceKernelThreadInfo> pInfo)
{
	sceLibKernel.todo("sceKernelGetThreadInfo(threadId=0x%x, pInfo=*0x%x)", threadId, pInfo);

	throw EXCEPTION("");
}

s32 sceKernelGetThreadRunStatus(vm::ptr<SceKernelThreadRunStatus> pStatus)
{
	sceLibKernel.todo("sceKernelGetThreadRunStatus(pStatus=*0x%x)", pStatus);

	throw EXCEPTION("");
}

s32 sceKernelGetSystemInfo(vm::ptr<SceKernelSystemInfo> pInfo)
{
	sceLibKernel.todo("sceKernelGetSystemInfo(pInfo=*0x%x)", pInfo);

	throw EXCEPTION("");
}

arm_error_code sceKernelGetThreadmgrUIDClass(s32 uid)
{
	sceLibKernel.error("sceKernelGetThreadmgrUIDClass(uid=0x%x)", uid);

	if (idm::check<ARMv7Thread>(uid)) return SCE_KERNEL_THREADMGR_UID_CLASS_THREAD;
	if (idm::check<psv_semaphore_t>(uid)) return SCE_KERNEL_THREADMGR_UID_CLASS_SEMA;
	if (idm::check<psv_event_flag_t>(uid)) return SCE_KERNEL_THREADMGR_UID_CLASS_EVENT_FLAG;
	if (idm::check<psv_mutex_t>(uid)) return SCE_KERNEL_THREADMGR_UID_CLASS_MUTEX;
	if (idm::check<psv_cond_t>(uid)) return SCE_KERNEL_THREADMGR_UID_CLASS_COND;
	
	return SCE_KERNEL_ERROR_INVALID_UID;
}

s32 sceKernelChangeThreadVfpException(s32 clearMask, s32 setMask)
{
	sceLibKernel.todo("sceKernelChangeThreadVfpException(clearMask=0x%x, setMask=0x%x)", clearMask, setMask);

	throw EXCEPTION("");
}

s32 sceKernelGetCurrentThreadVfpException()
{
	sceLibKernel.todo("sceKernelGetCurrentThreadVfpException()");

	throw EXCEPTION("");
}

s32 sceKernelDelayThread(u32 usec)
{
	sceLibKernel.todo("sceKernelDelayThread()");

	throw EXCEPTION("");
}

s32 sceKernelDelayThreadCB(u32 usec)
{
	sceLibKernel.todo("sceKernelDelayThreadCB()");

	throw EXCEPTION("");
}

arm_error_code sceKernelWaitThreadEnd(s32 threadId, vm::ptr<s32> pExitStatus, vm::ptr<u32> pTimeout)
{
	sceLibKernel.warning("sceKernelWaitThreadEnd(threadId=0x%x, pExitStatus=*0x%x, pTimeout=*0x%x)", threadId, pExitStatus, pTimeout);

	const auto thread = idm::get<ARMv7Thread>(threadId);

	if (!thread)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	if (pTimeout)
	{
	}

	while (!(thread->state & cpu_state::exit))
	{
		CHECK_EMU_STATUS;

		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
	}

	if (pExitStatus)
	{
		*pExitStatus = thread->GPR[0];
	}

	return SCE_OK;
}

s32 sceKernelWaitThreadEndCB(s32 threadId, vm::ptr<s32> pExitStatus, vm::ptr<u32> pTimeout)
{
	sceLibKernel.todo("sceKernelWaitThreadEndCB(threadId=0x%x, pExitStatus=*0x%x, pTimeout=*0x%x)", threadId, pExitStatus, pTimeout);

	throw EXCEPTION("");
}

// Callback functions

s32 sceKernelCreateCallback(vm::cptr<char> pName, u32 attr, vm::ptr<SceKernelCallbackFunction> callbackFunc, vm::ptr<void> pCommon)
{
	throw EXCEPTION("");
}

s32 sceKernelDeleteCallback(s32 callbackId)
{
	throw EXCEPTION("");
}

s32 sceKernelNotifyCallback(s32 callbackId, s32 notifyArg)
{
	throw EXCEPTION("");
}

s32 sceKernelCancelCallback(s32 callbackId)
{
	throw EXCEPTION("");
}

s32 sceKernelGetCallbackCount(s32 callbackId)
{
	throw EXCEPTION("");
}

s32 sceKernelCheckCallback()
{
	throw EXCEPTION("");
}

s32 sceKernelGetCallbackInfo(s32 callbackId, vm::ptr<SceKernelCallbackInfo> pInfo)
{
	throw EXCEPTION("");
}

s32 sceKernelRegisterCallbackToEvent(s32 eventId, s32 callbackId)
{
	throw EXCEPTION("");
}

s32 sceKernelUnregisterCallbackFromEvent(s32 eventId, s32 callbackId)
{
	throw EXCEPTION("");
}

s32 sceKernelUnregisterCallbackFromEventAll(s32 eventId)
{
	throw EXCEPTION("");
}

// Event functions

s32 sceKernelWaitEvent(s32 eventId, u32 waitPattern, vm::ptr<u32> pResultPattern, vm::ptr<u64> pUserData, vm::ptr<u32> pTimeout)
{
	throw EXCEPTION("");
}

s32 sceKernelWaitEventCB(s32 eventId, u32 waitPattern, vm::ptr<u32> pResultPattern, vm::ptr<u64> pUserData, vm::ptr<u32> pTimeout)
{
	throw EXCEPTION("");
}

s32 sceKernelPollEvent(s32 eventId, u32 bitPattern, vm::ptr<u32> pResultPattern, vm::ptr<u64> pUserData)
{
	throw EXCEPTION("");
}

s32 sceKernelCancelEvent(s32 eventId, vm::ptr<s32> pNumWaitThreads)
{
	throw EXCEPTION("");
}

s32 sceKernelGetEventInfo(s32 eventId, vm::ptr<SceKernelEventInfo> pInfo)
{
	throw EXCEPTION("");
}

s32 sceKernelWaitMultipleEvents(vm::ptr<SceKernelWaitEvent> pWaitEventList, s32 numEvents, u32 waitMode, vm::ptr<SceKernelResultEvent> pResultEventList, vm::ptr<u32> pTimeout)
{
	throw EXCEPTION("");
}

s32 sceKernelWaitMultipleEventsCB(vm::ptr<SceKernelWaitEvent> pWaitEventList, s32 numEvents, u32 waitMode, vm::ptr<SceKernelResultEvent> pResultEventList, vm::ptr<u32> pTimeout)
{
	throw EXCEPTION("");
}

// Event flag functions

arm_error_code sceKernelCreateEventFlag(vm::cptr<char> pName, u32 attr, u32 initPattern, vm::cptr<SceKernelEventFlagOptParam> pOptParam)
{
	sceLibKernel.error("sceKernelCreateEventFlag(pName=*0x%x, attr=0x%x, initPattern=0x%x, pOptParam=*0x%x)", pName, attr, initPattern, pOptParam);

	return NOT_AN_ERROR(idm::make<psv_event_flag_t>(pName.get_ptr(), attr, initPattern));
}

arm_error_code sceKernelDeleteEventFlag(s32 evfId)
{
	sceLibKernel.error("sceKernelDeleteEventFlag(evfId=0x%x)", evfId);

	const auto evf = idm::withdraw<psv_event_flag_t>(evfId);

	if (!evf)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	// Unregister IPC name
	if (evf->ref.atomic_op(ipc_ref_try_unregister))
	{
		evf->destroy();
	}

	return SCE_OK;
}

arm_error_code sceKernelOpenEventFlag(vm::cptr<char> pName)
{
	sceLibKernel.error("sceKernelOpenEventFlag(pName=*0x%x)", pName);

	// For now, go through all objects to find the name
	for (const auto& data : idm::get_map<psv_event_flag_t>())
	{
		const auto& evf = data.second;

		if (evf->name == pName.get_ptr() && evf->ref.atomic_op(ipc_ref_try_inc))
		{
			return NOT_AN_ERROR(idm::import_existing(evf));
		}
	}

	return SCE_KERNEL_ERROR_UID_CANNOT_FIND_BY_NAME;
}

arm_error_code sceKernelCloseEventFlag(s32 evfId)
{
	sceLibKernel.error("sceKernelCloseEventFlag(evfId=0x%x)", evfId);

	const auto evf = idm::withdraw<psv_event_flag_t>(evfId);

	if (!evf)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	// Decrement IPC ref
	if (evf->ref.atomic_op(ipc_ref_try_dec))
	{
		evf->destroy();
	}

	return SCE_OK;
}

arm_error_code sceKernelWaitEventFlag(ARMv7Thread& cpu, s32 evfId, u32 bitPattern, u32 waitMode, vm::ptr<u32> pResultPat, vm::ptr<u32> pTimeout)
{
	sceLibKernel.error("sceKernelWaitEventFlag(evfId=0x%x, bitPattern=0x%x, waitMode=0x%x, pResultPat=*0x%x, pTimeout=*0x%x)", evfId, bitPattern, waitMode, pResultPat, pTimeout);

	const u64 start_time = pTimeout ? get_system_time() : 0;
	const u32 timeout = pTimeout ? pTimeout->value() : 0;

	const auto evf = idm::get<psv_event_flag_t>(evfId);

	if (!evf)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	std::unique_lock<std::mutex> lock(evf->mutex);

	const u32 result = evf->pattern.fetch_op(event_flag_try_poll, bitPattern, waitMode);

	if (event_flag_test(result, bitPattern, waitMode))
	{
		if (pResultPat) *pResultPat = result;

		return SCE_OK;
	}

	// fixup register values for external use
	cpu.GPR[1] = bitPattern;
	cpu.GPR[2] = waitMode;

	// add waiter; attributes are ignored in current implementation
	sleep_entry<cpu_thread> waiter(evf->sq, cpu);

	while (!cpu.state.test_and_reset(cpu_state::signal))
	{
		CHECK_EMU_STATUS;

		if (pTimeout)
		{
			const u64 passed = get_system_time() - start_time;

			if (passed >= timeout)
			{
				cpu.GPR[0] = SCE_KERNEL_ERROR_WAIT_TIMEOUT;
				cpu.GPR[1] = evf->pattern;
				break;
			}

			get_current_thread_cv().wait_for(lock, std::chrono::microseconds(timeout - passed));
		}
		else
		{
			get_current_thread_cv().wait(lock);
		}
	}

	if (pResultPat) *pResultPat = cpu.GPR[1];
	if (pTimeout) *pTimeout = static_cast<u32>(std::max<s64>(0, timeout - (get_system_time() - start_time)));

	return NOT_AN_ERROR(cpu.GPR[0]);
}

arm_error_code sceKernelWaitEventFlagCB(ARMv7Thread& cpu, s32 evfId, u32 bitPattern, u32 waitMode, vm::ptr<u32> pResultPat, vm::ptr<u32> pTimeout)
{
	sceLibKernel.todo("sceKernelWaitEventFlagCB(evfId=0x%x, bitPattern=0x%x, waitMode=0x%x, pResultPat=*0x%x, pTimeout=*0x%x)", evfId, bitPattern, waitMode, pResultPat, pTimeout);

	return sceKernelWaitEventFlag(cpu, evfId, bitPattern, waitMode, pResultPat, pTimeout);
}

arm_error_code sceKernelPollEventFlag(s32 evfId, u32 bitPattern, u32 waitMode, vm::ptr<u32> pResultPat)
{
	sceLibKernel.error("sceKernelPollEventFlag(evfId=0x%x, bitPattern=0x%x, waitMode=0x%x, pResultPat=*0x%x)", evfId, bitPattern, waitMode, pResultPat);

	const auto evf = idm::get<psv_event_flag_t>(evfId);

	if (!evf)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	std::lock_guard<std::mutex> lock(evf->mutex);

	const u32 result = evf->pattern.fetch_op(event_flag_try_poll, bitPattern, waitMode);

	if (!event_flag_test(result, bitPattern, waitMode))
	{
		return SCE_KERNEL_ERROR_EVENT_COND;
	}

	*pResultPat = result;

	return SCE_OK;
}

arm_error_code sceKernelSetEventFlag(s32 evfId, u32 bitPattern)
{
	sceLibKernel.error("sceKernelSetEventFlag(evfId=0x%x, bitPattern=0x%x)", evfId, bitPattern);

	const auto evf = idm::get<psv_event_flag_t>(evfId);

	if (!evf)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	std::lock_guard<std::mutex> lock(evf->mutex);

	evf->pattern |= bitPattern;

	auto pred = [&](cpu_thread* thread) -> bool
	{
		auto& cpu = static_cast<ARMv7Thread&>(*thread);

		// load pattern and mode from registers
		const u32 pattern = cpu.GPR[1];
		const u32 mode = cpu.GPR[2];

		// check specific pattern
		const u32 result = evf->pattern.fetch_op(event_flag_try_poll, pattern, mode);

		if (event_flag_test(result, pattern, mode))
		{
			// save pattern
			cpu.GPR[0] = SCE_OK;
			cpu.GPR[1] = result;

			thread->state += cpu_state::signal;
			thread->notify();
			return true;
		}

		return false;
	};

	// check all waiters; protocol is ignored in current implementation
	evf->sq.erase(std::remove_if(evf->sq.begin(), evf->sq.end(), pred), evf->sq.end());

	return SCE_OK;
}

arm_error_code sceKernelClearEventFlag(s32 evfId, u32 bitPattern)
{
	sceLibKernel.error("sceKernelClearEventFlag(evfId=0x%x, bitPattern=0x%x)", evfId, bitPattern);

	const auto evf = idm::get<psv_event_flag_t>(evfId);

	if (!evf)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	std::lock_guard<std::mutex> lock(evf->mutex);

	evf->pattern &= ~bitPattern;

	return SCE_OK;
}

arm_error_code sceKernelCancelEventFlag(s32 evfId, u32 setPattern, vm::ptr<s32> pNumWaitThreads)
{
	sceLibKernel.error("sceKernelCancelEventFlag(evfId=0x%x, setPattern=0x%x, pNumWaitThreads=*0x%x)", evfId, setPattern, pNumWaitThreads);

	const auto evf = idm::get<psv_event_flag_t>(evfId);

	if (!evf)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	std::lock_guard<std::mutex> lock(evf->mutex);

	for (auto& thread : evf->sq)
	{
		static_cast<ARMv7Thread&>(*thread).GPR[0] = SCE_KERNEL_ERROR_WAIT_CANCEL;
		static_cast<ARMv7Thread&>(*thread).GPR[1] = setPattern;
		thread->state += cpu_state::signal;
		thread->notify();
	}

	*pNumWaitThreads = static_cast<u32>(evf->sq.size());

	evf->pattern = setPattern;
	evf->sq.clear();

	return SCE_OK;
}

arm_error_code sceKernelGetEventFlagInfo(s32 evfId, vm::ptr<SceKernelEventFlagInfo> pInfo)
{
	sceLibKernel.error("sceKernelGetEventFlagInfo(evfId=0x%x, pInfo=*0x%x)", evfId, pInfo);

	const auto evf = idm::get<psv_event_flag_t>(evfId);

	if (!evf)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	std::lock_guard<std::mutex> lock(evf->mutex);

	pInfo->size = SIZE_32(SceKernelEventFlagInfo);
	pInfo->evfId = evfId;
	
	strcpy_trunc(pInfo->name, evf->name);

	pInfo->attr = evf->attr;
	pInfo->initPattern = evf->init;
	pInfo->currentPattern = evf->pattern;
	pInfo->numWaitThreads = static_cast<u32>(evf->sq.size());

	return SCE_OK;
}

// Semaphore functions

arm_error_code sceKernelCreateSema(vm::cptr<char> pName, u32 attr, s32 initCount, s32 maxCount, vm::cptr<SceKernelSemaOptParam> pOptParam)
{
	sceLibKernel.error("sceKernelCreateSema(pName=*0x%x, attr=0x%x, initCount=%d, maxCount=%d, pOptParam=*0x%x)", pName, attr, initCount, maxCount, pOptParam);

	return NOT_AN_ERROR(idm::make<psv_semaphore_t>(pName.get_ptr(), attr, initCount, maxCount));
}

arm_error_code sceKernelDeleteSema(s32 semaId)
{
	sceLibKernel.error("sceKernelDeleteSema(semaId=0x%x)", semaId);

	const auto sema = idm::withdraw<psv_semaphore_t>(semaId);

	if (!sema)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	// ...

	return SCE_OK;
}

s32 sceKernelOpenSema(vm::cptr<char> pName)
{
	throw EXCEPTION("");
}

s32 sceKernelCloseSema(s32 semaId)
{
	throw EXCEPTION("");
}

arm_error_code sceKernelWaitSema(s32 semaId, s32 needCount, vm::ptr<u32> pTimeout)
{
	sceLibKernel.error("sceKernelWaitSema(semaId=0x%x, needCount=%d, pTimeout=*0x%x)", semaId, needCount, pTimeout);

	const auto sema = idm::get<psv_semaphore_t>(semaId);

	if (!sema)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	sceLibKernel.error("*** name = %s", sema->name);
	Emu.Pause();
	return SCE_OK;
}

s32 sceKernelWaitSemaCB(s32 semaId, s32 needCount, vm::ptr<u32> pTimeout)
{
	throw EXCEPTION("");
}

s32 sceKernelPollSema(s32 semaId, s32 needCount)
{
	throw EXCEPTION("");
}

s32 sceKernelSignalSema(s32 semaId, s32 signalCount)
{
	throw EXCEPTION("");
}

s32 sceKernelCancelSema(s32 semaId, s32 setCount, vm::ptr<s32> pNumWaitThreads)
{
	throw EXCEPTION("");
}

s32 sceKernelGetSemaInfo(s32 semaId, vm::ptr<SceKernelSemaInfo> pInfo)
{
	throw EXCEPTION("");
}

// Mutex functions

arm_error_code sceKernelCreateMutex(vm::cptr<char> pName, u32 attr, s32 initCount, vm::cptr<SceKernelMutexOptParam> pOptParam)
{
	sceLibKernel.error("sceKernelCreateMutex(pName=*0x%x, attr=0x%x, initCount=%d, pOptParam=*0x%x)", pName, attr, initCount, pOptParam);

	return NOT_AN_ERROR(idm::make<psv_mutex_t>(pName.get_ptr(), attr, initCount));
}

arm_error_code sceKernelDeleteMutex(s32 mutexId)
{
	sceLibKernel.error("sceKernelDeleteMutex(mutexId=0x%x)", mutexId);

	const auto mutex = idm::withdraw<psv_mutex_t>(mutexId);

	if (!mutex)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	// ...

	return SCE_OK;
}

s32 sceKernelOpenMutex(vm::cptr<char> pName)
{
	throw EXCEPTION("");
}

s32 sceKernelCloseMutex(s32 mutexId)
{
	throw EXCEPTION("");
}

s32 sceKernelLockMutex(s32 mutexId, s32 lockCount, vm::ptr<u32> pTimeout)
{
	throw EXCEPTION("");
}

s32 sceKernelLockMutexCB(s32 mutexId, s32 lockCount, vm::ptr<u32> pTimeout)
{
	throw EXCEPTION("");
}

s32 sceKernelTryLockMutex(s32 mutexId, s32 lockCount)
{
	throw EXCEPTION("");
}

s32 sceKernelUnlockMutex(s32 mutexId, s32 unlockCount)
{
	throw EXCEPTION("");
}

s32 sceKernelCancelMutex(s32 mutexId, s32 newCount, vm::ptr<s32> pNumWaitThreads)
{
	throw EXCEPTION("");
}

s32 sceKernelGetMutexInfo(s32 mutexId, vm::ptr<SceKernelMutexInfo> pInfo)
{
	throw EXCEPTION("");
}

// Lightweight mutex functions

s32 sceKernelCreateLwMutex(vm::ptr<SceKernelLwMutexWork> pWork, vm::cptr<char> pName, u32 attr, s32 initCount, vm::cptr<SceKernelLwMutexOptParam> pOptParam)
{
	throw EXCEPTION("");
}

s32 sceKernelDeleteLwMutex(vm::ptr<SceKernelLwMutexWork> pWork)
{
	throw EXCEPTION("");
}

s32 sceKernelLockLwMutex(vm::ptr<SceKernelLwMutexWork> pWork, s32 lockCount, vm::ptr<u32> pTimeout)
{
	throw EXCEPTION("");
}

s32 sceKernelLockLwMutexCB(vm::ptr<SceKernelLwMutexWork> pWork, s32 lockCount, vm::ptr<u32> pTimeout)
{
	throw EXCEPTION("");
}

s32 sceKernelTryLockLwMutex(vm::ptr<SceKernelLwMutexWork> pWork, s32 lockCount)
{
	throw EXCEPTION("");
}

s32 sceKernelUnlockLwMutex(vm::ptr<SceKernelLwMutexWork> pWork, s32 unlockCount)
{
	throw EXCEPTION("");
}

s32 sceKernelGetLwMutexInfo(vm::ptr<SceKernelLwMutexWork> pWork, vm::ptr<SceKernelLwMutexInfo> pInfo)
{
	throw EXCEPTION("");
}

s32 sceKernelGetLwMutexInfoById(s32 lwMutexId, vm::ptr<SceKernelLwMutexInfo> pInfo)
{
	throw EXCEPTION("");
}

// Condition variable functions

arm_error_code sceKernelCreateCond(vm::cptr<char> pName, u32 attr, s32 mutexId, vm::cptr<SceKernelCondOptParam> pOptParam)
{
	sceLibKernel.error("sceKernelCreateCond(pName=*0x%x, attr=0x%x, mutexId=0x%x, pOptParam=*0x%x)", pName, attr, mutexId, pOptParam);

	const auto mutex = idm::get<psv_mutex_t>(mutexId);

	if (!mutex)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	return NOT_AN_ERROR(idm::make<psv_cond_t>(pName.get_ptr(), attr, mutex));
}

arm_error_code sceKernelDeleteCond(s32 condId)
{
	sceLibKernel.error("sceKernelDeleteCond(condId=0x%x)", condId);

	const auto cond = idm::withdraw<psv_cond_t>(condId);

	if (!cond)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	// ...

	return SCE_OK;
}

s32 sceKernelOpenCond(vm::cptr<char> pName)
{
	throw EXCEPTION("");
}

s32 sceKernelCloseCond(s32 condId)
{
	throw EXCEPTION("");
}

s32 sceKernelWaitCond(s32 condId, vm::ptr<u32> pTimeout)
{
	throw EXCEPTION("");
}

s32 sceKernelWaitCondCB(s32 condId, vm::ptr<u32> pTimeout)
{
	throw EXCEPTION("");
}

s32 sceKernelSignalCond(s32 condId)
{
	throw EXCEPTION("");
}

s32 sceKernelSignalCondAll(s32 condId)
{
	throw EXCEPTION("");
}

s32 sceKernelSignalCondTo(s32 condId, s32 threadId)
{
	throw EXCEPTION("");
}

s32 sceKernelGetCondInfo(s32 condId, vm::ptr<SceKernelCondInfo> pInfo)
{
	throw EXCEPTION("");
}

// Lightweight condition variable functions

s32 sceKernelCreateLwCond(vm::ptr<SceKernelLwCondWork> pWork, vm::cptr<char> pName, u32 attr, vm::ptr<SceKernelLwMutexWork> pLwMutex, vm::cptr<SceKernelLwCondOptParam> pOptParam)
{
	throw EXCEPTION("");
}

s32 sceKernelDeleteLwCond(vm::ptr<SceKernelLwCondWork> pWork)
{
	throw EXCEPTION("");
}

s32 sceKernelWaitLwCond(vm::ptr<SceKernelLwCondWork> pWork, vm::ptr<u32> pTimeout)
{
	throw EXCEPTION("");
}

s32 sceKernelWaitLwCondCB(vm::ptr<SceKernelLwCondWork> pWork, vm::ptr<u32> pTimeout)
{
	throw EXCEPTION("");
}

s32 sceKernelSignalLwCond(vm::ptr<SceKernelLwCondWork> pWork)
{
	throw EXCEPTION("");
}

s32 sceKernelSignalLwCondAll(vm::ptr<SceKernelLwCondWork> pWork)
{
	throw EXCEPTION("");
}

s32 sceKernelSignalLwCondTo(vm::ptr<SceKernelLwCondWork> pWork, s32 threadId)
{
	throw EXCEPTION("");
}

s32 sceKernelGetLwCondInfo(vm::ptr<SceKernelLwCondWork> pWork, vm::ptr<SceKernelLwCondInfo> pInfo)
{
	throw EXCEPTION("");
}

s32 sceKernelGetLwCondInfoById(s32 lwCondId, vm::ptr<SceKernelLwCondInfo> pInfo)
{
	throw EXCEPTION("");
}

// Time functions

s32 sceKernelGetSystemTime(vm::ptr<SceKernelSysClock> pClock)
{
	throw EXCEPTION("");
}

u64 sceKernelGetSystemTimeWide()
{
	throw EXCEPTION("");
}

u32 sceKernelGetSystemTimeLow()
{
	throw EXCEPTION("");
}

// Timer functions

s32 sceKernelCreateTimer(vm::cptr<char> pName, u32 attr, vm::cptr<SceKernelTimerOptParam> pOptParam)
{
	throw EXCEPTION("");
}

s32 sceKernelDeleteTimer(s32 timerId)
{
	throw EXCEPTION("");
}

s32 sceKernelOpenTimer(vm::cptr<char> pName)
{
	throw EXCEPTION("");
}

s32 sceKernelCloseTimer(s32 timerId)
{
	throw EXCEPTION("");
}

s32 sceKernelStartTimer(s32 timerId)
{
	throw EXCEPTION("");
}

s32 sceKernelStopTimer(s32 timerId)
{
	throw EXCEPTION("");
}

s32 sceKernelGetTimerBase(s32 timerId, vm::ptr<SceKernelSysClock> pBase)
{
	throw EXCEPTION("");
}

u64 sceKernelGetTimerBaseWide(s32 timerId)
{
	throw EXCEPTION("");
}

s32 sceKernelGetTimerTime(s32 timerId, vm::ptr<SceKernelSysClock> pClock)
{
	throw EXCEPTION("");
}

u64 sceKernelGetTimerTimeWide(s32 timerId)
{
	throw EXCEPTION("");
}

s32 sceKernelSetTimerTime(s32 timerId, vm::ptr<SceKernelSysClock> pClock)
{
	throw EXCEPTION("");
}

u64 sceKernelSetTimerTimeWide(s32 timerId, u64 clock)
{
	throw EXCEPTION("");
}

s32 sceKernelSetTimerEvent(s32 timerId, s32 type, vm::ptr<SceKernelSysClock> pInterval, s32 fRepeat)
{
	throw EXCEPTION("");
}

s32 sceKernelCancelTimer(s32 timerId, vm::ptr<s32> pNumWaitThreads)
{
	throw EXCEPTION("");
}

s32 sceKernelGetTimerInfo(s32 timerId, vm::ptr<SceKernelTimerInfo> pInfo)
{
	throw EXCEPTION("");
}

// Reader/writer lock functions

s32 sceKernelCreateRWLock(vm::cptr<char> pName, u32 attr, vm::cptr<SceKernelRWLockOptParam> pOptParam)
{
	throw EXCEPTION("");
}

s32 sceKernelDeleteRWLock(s32 rwLockId)
{
	throw EXCEPTION("");
}

s32 sceKernelOpenRWLock(vm::cptr<char> pName)
{
	throw EXCEPTION("");
}

s32 sceKernelCloseRWLock(s32 rwLockId)
{
	throw EXCEPTION("");
}

s32 sceKernelLockReadRWLock(s32 rwLockId, vm::ptr<u32> pTimeout)
{
	throw EXCEPTION("");
}

s32 sceKernelLockReadRWLockCB(s32 rwLockId, vm::ptr<u32> pTimeout)
{
	throw EXCEPTION("");
}

s32 sceKernelTryLockReadRWLock(s32 rwLockId)
{
	throw EXCEPTION("");
}

s32 sceKernelUnlockReadRWLock(s32 rwLockId)
{
	throw EXCEPTION("");
}

s32 sceKernelLockWriteRWLock(s32 rwLockId, vm::ptr<u32> pTimeout)
{
	throw EXCEPTION("");
}

s32 sceKernelLockWriteRWLockCB(s32 rwLockId, vm::ptr<u32> pTimeout)
{
	throw EXCEPTION("");
}

s32 sceKernelTryLockWriteRWLock(s32 rwLockId)
{
	throw EXCEPTION("");
}

s32 sceKernelUnlockWriteRWLock(s32 rwLockId)
{
	throw EXCEPTION("");
}

s32 sceKernelCancelRWLock(s32 rwLockId, vm::ptr<s32> pNumReadWaitThreads, vm::ptr<s32> pNumWriteWaitThreads, s32 flag)
{
	throw EXCEPTION("");
}

s32 sceKernelGetRWLockInfo(s32 rwLockId, vm::ptr<SceKernelRWLockInfo> pInfo)
{
	throw EXCEPTION("");
}

// IO/File functions

s32 sceIoRemove(vm::cptr<char> filename)
{
	throw EXCEPTION("");
}

s32 sceIoMkdir(vm::cptr<char> dirname, s32 mode)
{
	throw EXCEPTION("");
}

s32 sceIoRmdir(vm::cptr<char> dirname)
{
	throw EXCEPTION("");
}

s32 sceIoRename(vm::cptr<char> oldname, vm::cptr<char> newname)
{
	throw EXCEPTION("");
}

s32 sceIoDevctl(vm::cptr<char> devname, s32 cmd, vm::cptr<void> arg, u32 arglen, vm::ptr<void> bufp, u32 buflen)
{
	throw EXCEPTION("");
}

s32 sceIoSync(vm::cptr<char> devname, s32 flag)
{
	throw EXCEPTION("");
}

s32 sceIoOpen(vm::cptr<char> filename, s32 flag, s32 mode)
{
	throw EXCEPTION("");
}

s32 sceIoClose(s32 fd)
{
	throw EXCEPTION("");
}

s32 sceIoIoctl(s32 fd, s32 cmd, vm::cptr<void> argp, u32 arglen, vm::ptr<void> bufp, u32 buflen)
{
	throw EXCEPTION("");
}

s64 sceIoLseek(s32 fd, s64 offset, s32 whence)
{
	throw EXCEPTION("");
}

s32 sceIoLseek32(s32 fd, s32 offset, s32 whence)
{
	throw EXCEPTION("");
}

s32 sceIoRead(s32 fd, vm::ptr<void> buf, u32 nbyte)
{
	throw EXCEPTION("");
}

s32 sceIoWrite(s32 fd, vm::cptr<void> buf, u32 nbyte)
{
	throw EXCEPTION("");
}

s32 sceIoPread(s32 fd, vm::ptr<void> buf, u32 nbyte, s64 offset)
{
	throw EXCEPTION("");
}

s32 sceIoPwrite(s32 fd, vm::cptr<void> buf, u32 nbyte, s64 offset)
{
	throw EXCEPTION("");
}

s32 sceIoDopen(vm::cptr<char> dirname)
{
	throw EXCEPTION("");
}

s32 sceIoDclose(s32 fd)
{
	throw EXCEPTION("");
}

s32 sceIoDread(s32 fd, vm::ptr<SceIoDirent> buf)
{
	throw EXCEPTION("");
}

s32 sceIoChstat(vm::cptr<char> name, vm::cptr<SceIoStat> buf, u32 cbit)
{
	throw EXCEPTION("");
}

s32 sceIoGetstat(vm::cptr<char> name, vm::ptr<SceIoStat> buf)
{
	throw EXCEPTION("");
}


#define REG_FUNC(nid, name) REG_FNID(SceLibKernel, nid, name)

DECLARE(arm_module_manager::SceLibKernel)("SceLibKernel", []()
{
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

	//REG_FUNC(0x4C4672BF, sceKernelGetProcessTime); // !!!
});

DECLARE(arm_module_manager::SceIofilemgr)("SceIofilemgr", []()
{
	REG_FNID(SceIofilemgr, 0x34EFD876, sceIoWrite); // !!!
	REG_FNID(SceIofilemgr, 0xC70B8886, sceIoClose); // !!!
	REG_FNID(SceIofilemgr, 0xFDB32293, sceIoRead); // !!!
});

DECLARE(arm_module_manager::SceModulemgr)("SceModulemgr", []()
{
	//REG_FNID(SceModulemgr, 0x36585DAF, sceKernelGetModuleInfo);
	//REG_FNID(SceModulemgr, 0x2EF2581F, sceKernelGetModuleList);
	//REG_FNID(SceModulemgr, 0xF5798C7C, sceKernelGetModuleIdByAddr);
});

DECLARE(arm_module_manager::SceProcessmgr)("SceProcessmgr", []()
{
	//REG_FNID(SceProcessmgr, 0xCD248267, sceKernelGetCurrentProcess);
	//REG_FNID(SceProcessmgr, 0x2252890C, sceKernelPowerTick);
	//REG_FNID(SceProcessmgr, 0x9E45DA09, sceKernelLibcClock);
	//REG_FNID(SceProcessmgr, 0x0039BE45, sceKernelLibcTime);
	//REG_FNID(SceProcessmgr, 0x4B879059, sceKernelLibcGettimeofday);
	//REG_FNID(SceProcessmgr, 0xC1727F59, sceKernelGetStdin);
	//REG_FNID(SceProcessmgr, 0xE5AA625C, sceKernelGetStdout);
	//REG_FNID(SceProcessmgr, 0xFA5E3ADA, sceKernelGetStderr);
	//REG_FNID(SceProcessmgr, 0xE6E9FCA3, sceKernelGetRemoteProcessTime);
	//REG_FNID(SceProcessmgr, 0xD37A8437, sceKernelGetProcessTime);
	//REG_FNID(SceProcessmgr, 0xF5D0D4C6, sceKernelGetProcessTimeLow);
	//REG_FNID(SceProcessmgr, 0x89DA0967, sceKernelGetProcessTimeWide);
	//REG_FNID(SceProcessmgr, 0x2BE3E066, sceKernelGetProcessParam);
});

DECLARE(arm_module_manager::SceStdio)("SceStdio", []()
{
	//REG_FNID(SceStdio, 0x54237407, sceKernelStdin);
	//REG_FNID(SceStdio, 0x9033E9BD, sceKernelStdout);
	//REG_FNID(SceStdio, 0x35EE7CF5, sceKernelStderr);
});

DECLARE(arm_module_manager::SceSysmem)("SceSysmem", []()
{
	REG_FNID(SceSysmem, 0xB9D5EBDE, sceKernelAllocMemBlock);
	REG_FNID(SceSysmem, 0xA91E15EE, sceKernelFreeMemBlock);
	REG_FNID(SceSysmem, 0xB8EF5818, sceKernelGetMemBlockBase);
	//REG_FNID(SceSysmem, 0x3B29E0F5, sceKernelRemapMemBlock);
	//REG_FNID(SceSysmem, 0xA33B99D1, sceKernelFindMemBlockByAddr);
	REG_FNID(SceSysmem, 0x4010AD65, sceKernelGetMemBlockInfoByAddr);
});

DECLARE(arm_module_manager::SceCpu)("SceCpu", []()
{
	//REG_FNID(SceCpu, 0x2704CFEE, sceKernelCpuId);
});

DECLARE(arm_module_manager::SceDipsw)("SceDipsw", []()
{
	//REG_FNID(SceDipsw, 0x1C783FB2, sceKernelCheckDipsw);
	//REG_FNID(SceDipsw, 0x817053D4, sceKernelSetDipsw);
	//REG_FNID(SceDipsw, 0x800EDCC1, sceKernelClearDipsw);
});

DECLARE(arm_module_manager::SceThreadmgr)("SceThreadmgr", []()
{
	REG_FNID(SceThreadmgr, 0x0C8A38E1, sceKernelExitThread);
	REG_FNID(SceThreadmgr, 0x1D17DECF, sceKernelExitDeleteThread);
	REG_FNID(SceThreadmgr, 0x4B675D05, sceKernelDelayThread);
	REG_FNID(SceThreadmgr, 0x9C0180E1, sceKernelDelayThreadCB);
	REG_FNID(SceThreadmgr, 0x1BBDE3D9, sceKernelDeleteThread); // !!!
	//REG_FNID(SceThreadmgr, 0x001173F8, sceKernelChangeActiveCpuMask);
	REG_FNID(SceThreadmgr, 0x01414F0B, sceKernelGetThreadCurrentPriority);
	REG_FNID(SceThreadmgr, 0x751C9B7A, sceKernelChangeCurrentThreadAttr);
	REG_FNID(SceThreadmgr, 0xD9BD74EB, sceKernelCheckWaitableStatus);
	REG_FNID(SceThreadmgr, 0x9DCB4B7A, sceKernelGetProcessId);
	REG_FNID(SceThreadmgr, 0xB19CF7E9, sceKernelCreateCallback); // !!!
	REG_FNID(SceThreadmgr, 0xD469676B, sceKernelDeleteCallback); // !!!
	REG_FNID(SceThreadmgr, 0xE53E41F6, sceKernelCheckCallback);
	REG_FNID(SceThreadmgr, 0xF4EE4FA9, sceKernelGetSystemTimeWide);
	REG_FNID(SceThreadmgr, 0x47F6DE49, sceKernelGetSystemTimeLow);
	//REG_FNID(SceThreadmgr, 0xC0FAF6A3, sceKernelCreateThreadForUser);
	REG_FNID(SceThreadmgr, 0xF1AE5654, sceKernelGetThreadCpuAffinityMask); // !!!
});

DECLARE(arm_module_manager::SceDebugLed)("SceDebugLed", []()
{
	//REG_FNID(SceDebugLed, 0x78E702D3, sceKernelSetGPO);
});
