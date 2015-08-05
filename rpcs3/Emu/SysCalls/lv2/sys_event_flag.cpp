#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/Cell/PPUThread.h"
#include "sys_event_flag.h"

SysCallBase sys_event_flag("sys_event_flag");

extern u64 get_system_time();

void lv2_event_flag_t::notify_all(lv2_lock_t& lv2_lock)
{
	CHECK_LV2_LOCK(lv2_lock);

	auto pred = [this](sleep_queue_t::value_type& thread) -> bool
	{
		auto& ppu = static_cast<PPUThread&>(*thread);

		// load pattern and mode from registers
		const u64 bitptn = ppu.GPR[4];
		const u32 mode = static_cast<u32>(ppu.GPR[5]);

		// check specific pattern
		if (check_pattern(bitptn, mode))
		{
			// save pattern
			ppu.GPR[4] = clear_pattern(bitptn, mode);

			if (!ppu.signal())
			{
				throw EXCEPTION("Thread already signaled");
			}

			return true;
		}

		return false;
	};

	// check all waiters; protocol is ignored in current implementation
	sq.erase(std::remove_if(sq.begin(), sq.end(), pred), sq.end());
}

s32 sys_event_flag_create(vm::ptr<u32> id, vm::ptr<sys_event_flag_attribute_t> attr, u64 init)
{
	sys_event_flag.Warning("sys_event_flag_create(id=*0x%x, attr=*0x%x, init=0x%llx)", id, attr, init);

	if (!id || !attr)
	{
		return CELL_EFAULT;
	}

	const u32 protocol = attr->protocol;

	if (protocol != SYS_SYNC_FIFO && protocol != SYS_SYNC_RETRY && protocol != SYS_SYNC_PRIORITY && protocol != SYS_SYNC_PRIORITY_INHERIT)
	{
		sys_event_flag.Error("sys_event_flag_create(): unknown protocol (0x%x)", protocol);
		return CELL_EINVAL;
	}

	if (attr->pshared != SYS_SYNC_NOT_PROCESS_SHARED || attr->ipc_key.data() || attr->flags.data())
	{
		sys_event_flag.Error("sys_event_flag_create(): unknown attributes (pshared=0x%x, ipc_key=0x%llx, flags=0x%x)", attr->pshared, attr->ipc_key, attr->flags);
		return CELL_EINVAL;
	}

	const u32 type = attr->type;

	if (type != SYS_SYNC_WAITER_SINGLE && type != SYS_SYNC_WAITER_MULTIPLE)
	{
		sys_event_flag.Error("sys_event_flag_create(): unknown type (0x%x)", type);
		return CELL_EINVAL;
	}

	*id = idm::make<lv2_event_flag_t>(init, protocol, type, attr->name_u64);

	return CELL_OK;
}

s32 sys_event_flag_destroy(u32 id)
{
	sys_event_flag.Warning("sys_event_flag_destroy(id=0x%x)", id);

	LV2_LOCK;

	const auto eflag = idm::get<lv2_event_flag_t>(id);

	if (!eflag)
	{
		return CELL_ESRCH;
	}

	if (!eflag->sq.empty())
	{
		return CELL_EBUSY;
	}

	idm::remove<lv2_event_flag_t>(id);

	return CELL_OK;
}

s32 sys_event_flag_wait(PPUThread& ppu, u32 id, u64 bitptn, u32 mode, vm::ptr<u64> result, u64 timeout)
{
	sys_event_flag.Log("sys_event_flag_wait(id=0x%x, bitptn=0x%llx, mode=0x%x, result=*0x%x, timeout=0x%llx)", id, bitptn, mode, result, timeout);

	const u64 start_time = get_system_time();

	// If this syscall is called through the SC instruction, these registers must already contain corresponding values.
	// But let's fixup them (in the case of explicit function call or something) because these values are used externally.
	ppu.GPR[4] = bitptn;
	ppu.GPR[5] = mode;

	LV2_LOCK;

	if (result) *result = 0; // This is very annoying.

	if (!lv2_event_flag_t::check_mode(mode))
	{
		sys_event_flag.Error("sys_event_flag_wait(): unknown mode (0x%x)", mode);
		return CELL_EINVAL;
	}

	const auto eflag = idm::get<lv2_event_flag_t>(id);

	if (!eflag)
	{
		return CELL_ESRCH;
	}

	if (eflag->type == SYS_SYNC_WAITER_SINGLE && eflag->sq.size() > 0)
	{
		return CELL_EPERM;
	}

	if (eflag->check_pattern(bitptn, mode))
	{
		const u64 pattern = eflag->clear_pattern(bitptn, mode);

		if (result) *result = pattern;

		return CELL_OK;
	}

	// add waiter; protocol is ignored in current implementation
	sleep_queue_entry_t waiter(ppu, eflag->sq);

	while (!ppu.unsignal())
	{
		CHECK_EMU_STATUS;

		if (timeout)
		{
			const u64 passed = get_system_time() - start_time;

			if (passed >= timeout)
			{
				if (result) *result = eflag->pattern;

				return CELL_ETIMEDOUT;
			}

			ppu.cv.wait_for(lv2_lock, std::chrono::microseconds(timeout - passed));
		}
		else
		{
			ppu.cv.wait(lv2_lock);
		}
	}
	
	// load pattern saved upon signaling
	if (result)
	{
		*result = ppu.GPR[4];
	}

	// check cause
	if (ppu.GPR[5] == 0)
	{
		return CELL_ECANCELED;
	}

	return CELL_OK;
}

s32 sys_event_flag_trywait(u32 id, u64 bitptn, u32 mode, vm::ptr<u64> result)
{
	sys_event_flag.Log("sys_event_flag_trywait(id=0x%x, bitptn=0x%llx, mode=0x%x, result=*0x%x)", id, bitptn, mode, result);

	LV2_LOCK;

	if (result) *result = 0; // This is very annoying.

	if (!lv2_event_flag_t::check_mode(mode))
	{
		sys_event_flag.Error("sys_event_flag_trywait(): unknown mode (0x%x)", mode);
		return CELL_EINVAL;
	}

	const auto eflag = idm::get<lv2_event_flag_t>(id);

	if (!eflag)
	{
		return CELL_ESRCH;
	}

	if (eflag->check_pattern(bitptn, mode))
	{
		const u64 pattern = eflag->clear_pattern(bitptn, mode);

		if (result) *result = pattern;

		return CELL_OK;
	}

	return CELL_EBUSY;
}

s32 sys_event_flag_set(u32 id, u64 bitptn)
{
	sys_event_flag.Log("sys_event_flag_set(id=0x%x, bitptn=0x%llx)", id, bitptn);

	LV2_LOCK;

	const auto eflag = idm::get<lv2_event_flag_t>(id);

	if (!eflag)
	{
		return CELL_ESRCH;
	}

	if (bitptn && ~eflag->pattern.fetch_or(bitptn) & bitptn)
	{
		eflag->notify_all(lv2_lock);
	}
	
	return CELL_OK;
}

s32 sys_event_flag_clear(u32 id, u64 bitptn)
{
	sys_event_flag.Log("sys_event_flag_clear(id=0x%x, bitptn=0x%llx)", id, bitptn);

	LV2_LOCK;

	const auto eflag = idm::get<lv2_event_flag_t>(id);

	if (!eflag)
	{
		return CELL_ESRCH;
	}

	eflag->pattern &= bitptn;

	return CELL_OK;
}

s32 sys_event_flag_cancel(u32 id, vm::ptr<u32> num)
{
	sys_event_flag.Log("sys_event_flag_cancel(id=0x%x, num=*0x%x)", id, num);

	LV2_LOCK;

	if (num)
	{
		*num = 0;
	}

	const auto eflag = idm::get<lv2_event_flag_t>(id);

	if (!eflag)
	{
		return CELL_ESRCH;
	}

	if (num)
	{
		*num = static_cast<u32>(eflag->sq.size());
	}

	const u64 pattern = eflag->pattern;

	// signal all threads to return CELL_ECANCELED
	for (auto& thread : eflag->sq)
	{
		auto& ppu = static_cast<PPUThread&>(*thread);

		// save existing pattern
		ppu.GPR[4] = pattern;

		// clear "mode" as a sign of cancellation
		ppu.GPR[5] = 0;

		if (!thread->signal())
		{
			throw EXCEPTION("Thread already signaled");
		}
	}

	eflag->sq.clear();
	
	return CELL_OK;
}

s32 sys_event_flag_get(u32 id, vm::ptr<u64> flags)
{
	sys_event_flag.Log("sys_event_flag_get(id=0x%x, flags=*0x%x)", id, flags);

	LV2_LOCK;

	if (!flags)
	{
		return CELL_EFAULT;
	}

	const auto eflag = idm::get<lv2_event_flag_t>(id);

	if (!eflag)
	{
		*flags = 0; // This is very annoying.

		return CELL_ESRCH;
	}

	*flags = eflag->pattern;

	return CELL_OK;
}
