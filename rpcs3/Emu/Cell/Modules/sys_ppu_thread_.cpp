#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/lv2/sys_ppu_thread.h"
#include "sysPrxForUser.h"

extern logs::channel sysPrxForUser;

extern u32 ppu_alloc_tls();
extern void ppu_free_tls(u32 addr);

s32 sys_ppu_thread_create(ppu_thread& ppu, vm::ptr<u64> thread_id, u32 entry, u64 arg, s32 prio, u32 stacksize, u64 flags, vm::cptr<char> threadname)
{
	sysPrxForUser.warning("sys_ppu_thread_create(thread_id=*0x%x, entry=0x%x, arg=0x%llx, prio=%d, stacksize=0x%x, flags=0x%llx, threadname=%s)",
		thread_id, entry, arg, prio, stacksize, flags, threadname);

	// Allocate TLS
	const u32 tls_addr = ppu_alloc_tls();

	if (!tls_addr)
	{
		return CELL_ENOMEM;
	}

	// Call the syscall
	if (s32 res = _sys_ppu_thread_create(thread_id, vm::make_var(ppu_thread_param_t{ entry, tls_addr + 0x7030 }), arg, 0, prio, stacksize, flags, threadname))
	{
		return res;
	}

	if (flags & SYS_PPU_THREAD_CREATE_INTERRUPT)
	{
		return CELL_OK;
	}

	// Run the thread
	if (s32 res = sys_ppu_thread_start(ppu, static_cast<u32>(*thread_id)))
	{
		return res;
	}

	return CELL_OK;
}

s32 sys_ppu_thread_get_id(ppu_thread& ppu, vm::ptr<u64> thread_id)
{
	sysPrxForUser.trace("sys_ppu_thread_get_id(thread_id=*0x%x)", thread_id);

	*thread_id = ppu.id;

	return CELL_OK;
}

void sys_ppu_thread_exit(ppu_thread& ppu, u64 val)
{
	sysPrxForUser.trace("sys_ppu_thread_exit(val=0x%llx)", val);

	// (call registered atexit functions)
	// ...
	
	// Deallocate TLS
	ppu_free_tls(vm::cast(ppu.gpr[13], HERE) - 0x7030);

	if (ppu.gpr[3] == val)
	{
		// Change sys_ppu_thread_exit code to the syscall code (hack)
		ppu.gpr[11] = 41;
	}

	// Call the syscall
	return _sys_ppu_thread_exit(ppu, val);
}

shared_mutex g_once_mutex;

void sys_ppu_thread_once(ppu_thread& ppu, vm::ptr<atomic_be_t<u32>> once_ctrl, vm::ptr<void()> init)
{
	sysPrxForUser.warning("sys_ppu_thread_once(once_ctrl=*0x%x, init=*0x%x)", once_ctrl, init);

	writer_lock lock(g_once_mutex);

	if (once_ctrl->compare_and_swap_test(SYS_PPU_THREAD_ONCE_INIT, SYS_PPU_THREAD_DONE_INIT))
	{
		// call init function using current thread context
		init(ppu);
	}
}

s32 sys_ppu_thread_register_atexit()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sys_ppu_thread_unregister_atexit()
{
	fmt::throw_exception("Unimplemented" HERE);
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
