#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "Emu/SysCalls/lv2/sys_ppu_thread.h"
#include "sysPrxForUser.h"

extern Module sysPrxForUser;

s32 sys_ppu_thread_create(PPUThread& ppu, vm::ptr<u64> thread_id, u32 entry, u64 arg, s32 prio, u32 stacksize, u64 flags, vm::cptr<char> threadname)
{
	sysPrxForUser.Warning("sys_ppu_thread_create(thread_id=*0x%x, entry=0x%x, arg=0x%llx, prio=%d, stacksize=0x%x, flags=0x%llx, threadname=*0x%x)", thread_id, entry, arg, prio, stacksize, flags, threadname);

	// (allocate TLS)
	// (return CELL_ENOMEM if failed)
	// ...

	vm::stackvar<ppu_thread_param_t> attr(ppu);

	attr->entry = entry;
	attr->tls = 0;

	// call the syscall
	if (s32 res = _sys_ppu_thread_create(thread_id, attr, arg, 0, prio, stacksize, flags, threadname))
	{
		return res;
	}

	// run the thread
	return flags & SYS_PPU_THREAD_CREATE_INTERRUPT ? CELL_OK : sys_ppu_thread_start(static_cast<u32>(*thread_id));
}

s32 sys_ppu_thread_get_id(PPUThread& ppu, vm::ptr<u64> thread_id)
{
	sysPrxForUser.Log("sys_ppu_thread_get_id(thread_id=*0x%x)", thread_id);

	*thread_id = ppu.get_id();

	return CELL_OK;
}

void sys_ppu_thread_exit(PPUThread& ppu, u64 val)
{
	sysPrxForUser.Log("sys_ppu_thread_exit(val=0x%llx)", val);

	// (call registered atexit functions)
	// (deallocate TLS)
	// ...

	// call the syscall
	_sys_ppu_thread_exit(ppu, val);
}

std::mutex g_once_mutex;

void sys_ppu_thread_once(PPUThread& ppu, vm::ptr<atomic_be_t<u32>> once_ctrl, vm::ptr<void()> init)
{
	sysPrxForUser.Warning("sys_ppu_thread_once(once_ctrl=*0x%x, init=*0x%x)", once_ctrl, init);

	std::lock_guard<std::mutex> lock(g_once_mutex);

	if (once_ctrl->compare_and_swap_test(SYS_PPU_THREAD_ONCE_INIT, SYS_PPU_THREAD_DONE_INIT))
	{
		// call init function using current thread context
		init(ppu);
	}
}

s32 sys_ppu_thread_register_atexit()
{
	throw EXCEPTION("");
}

s32 sys_ppu_thread_unregister_atexit()
{
	throw EXCEPTION("");
}

void sysPrxForUser_sys_ppu_thread_init()
{
	REG_FUNC(sysPrxForUser, sys_ppu_thread_create);
	REG_FUNC(sysPrxForUser, sys_ppu_thread_get_id);
	REG_FUNC(sysPrxForUser, sys_ppu_thread_exit);
	REG_FUNC(sysPrxForUser, sys_ppu_thread_once);
	REG_FUNC(sysPrxForUser, sys_ppu_thread_register_atexit);
	REG_FUNC(sysPrxForUser, sys_ppu_thread_unregister_atexit);
}
