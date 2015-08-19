#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "Emu/SysCalls/lv2/sys_interrupt.h"
#include "Emu/SysCalls/lv2/sys_process.h"
#include "sysPrxForUser.h"

extern Module sysPrxForUser;

extern u64 get_system_time();

#define TLS_MAX 128
#define TLS_SYS 0x30

u32 g_tls_start; // start of TLS memory area
u32 g_tls_size;

std::array<std::atomic<u32>, TLS_MAX> g_tls_owners;

void sys_initialize_tls()
{
	sysPrxForUser.Log("sys_initialize_tls()");
}

u32 ppu_get_tls(u32 thread)
{
	if (!g_tls_start)
	{
		g_tls_size = Emu.GetTLSMemsz() + TLS_SYS;
		g_tls_start = vm::alloc(g_tls_size * TLS_MAX, vm::main); // memory for up to TLS_MAX threads
		LOG_NOTICE(MEMORY, "Thread Local Storage initialized (g_tls_start=0x%x, user_size=0x%x)\n*** TLS segment addr: 0x%08x\n*** TLS segment size: 0x%08x",
			g_tls_start, Emu.GetTLSMemsz(), Emu.GetTLSAddr(), Emu.GetTLSFilesz());
	}

	if (!thread)
	{
		return 0;
	}
	
	for (u32 i = 0; i < TLS_MAX; i++)
	{
		if (g_tls_owners[i] == thread)
		{
			return g_tls_start + i * g_tls_size + TLS_SYS; // if already initialized, return TLS address
		}
	}

	for (u32 i = 0; i < TLS_MAX; i++)
	{
		u32 old = 0;
		if (g_tls_owners[i].compare_exchange_strong(old, thread))
		{
			const u32 addr = g_tls_start + i * g_tls_size + TLS_SYS; // get TLS address
			memset(vm::get_ptr(addr - TLS_SYS), 0, TLS_SYS); // initialize system area with zeros
			memcpy(vm::get_ptr(addr), vm::get_ptr(Emu.GetTLSAddr()), Emu.GetTLSFilesz()); // initialize from TLS image
			memset(vm::get_ptr(addr + Emu.GetTLSFilesz()), 0, Emu.GetTLSMemsz() - Emu.GetTLSFilesz()); // fill the rest with zeros
			return addr;
		}
	}

	throw EXCEPTION("Out of TLS memory");
}

void ppu_free_tls(u32 thread)
{
	for (auto& v : g_tls_owners)
	{
		u32 old = thread;
		if (v.compare_exchange_strong(old, 0))
		{
			return;
		}
	}

	LOG_ERROR(MEMORY, "TLS deallocation failed (thread=0x%x)", thread);
}

s64 sys_time_get_system_time()
{
	sysPrxForUser.Log("sys_time_get_system_time()");

	return get_system_time();
}

s64 _sys_process_atexitspawn()
{
	sysPrxForUser.Log("_sys_process_atexitspawn()");
	return CELL_OK;
}

s64 _sys_process_at_Exitspawn()
{
	sysPrxForUser.Log("_sys_process_at_Exitspawn");
	return CELL_OK;
}

s32 sys_interrupt_thread_disestablish(PPUThread& ppu, u32 ih)
{
	sysPrxForUser.Todo("sys_interrupt_thread_disestablish(ih=0x%x)", ih);

	return _sys_interrupt_thread_disestablish(ppu, ih, vm::var<u64>(ppu));
}

s32 sys_process_is_stack(u32 p)
{
	sysPrxForUser.Log("sys_process_is_stack(p=0x%x)", p);

	// prx: compare high 4 bits with "0xD"
	return (p >> 28) == 0xD;
}

s32 sys_process_get_paramsfo(vm::ptr<char> buffer)
{
	sysPrxForUser.Warning("sys_process_get_paramsfo(buffer=*0x%x)", buffer);

	// prx: load some data (0x40 bytes) previously set by _sys_process_get_paramsfo syscall
	return _sys_process_get_paramsfo(buffer);
}

s32 sys_get_random_number(vm::ptr<u8> addr, u64 size)
{
	sysPrxForUser.Warning("sys_get_random_number(addr=*0x%x, size=%d)", addr, size);

	if (size > 4096)
		size = 4096;

	for (u32 i = 0; i < (u32)size - 1; i++)
	{
		addr[i] = rand() % 256;
	}

	return CELL_OK;
}

s32 console_getc()
{
	throw EXCEPTION("");
}

s32 console_putc()
{
	throw EXCEPTION("");
}

s32 console_write()
{
	throw EXCEPTION("");
}


extern void sysPrxForUser_sys_lwmutex_init();
extern void sysPrxForUser_sys_lwcond_init();
extern void sysPrxForUser_sys_ppu_thread_init();
extern void sysPrxForUser_sys_prx_init();
extern void sysPrxForUser_sys_heap_init();
extern void sysPrxForUser_sys_spinlock_init();
extern void sysPrxForUser_sys_mmapper_init();
extern void sysPrxForUser_sys_mempool_init();
extern void sysPrxForUser_sys_spu_init();
extern void sysPrxForUser_sys_game_init();
extern void sysPrxForUser_sys_libc_init();

Module sysPrxForUser("sysPrxForUser", []()
{
	g_tls_start = 0;
	for (auto& v : g_tls_owners)
	{
		v.store(0, std::memory_order_relaxed);
	}

	// Setup random number generator
	srand(time(NULL));

	//REG_VARIABLE(sysPrxForUser, sys_prx_version); // 0x7df066cf

	sysPrxForUser_sys_lwmutex_init();
	sysPrxForUser_sys_lwcond_init();
	sysPrxForUser_sys_ppu_thread_init();
	sysPrxForUser_sys_prx_init();
	sysPrxForUser_sys_heap_init();
	sysPrxForUser_sys_spinlock_init();
	sysPrxForUser_sys_mmapper_init();
	sysPrxForUser_sys_mempool_init();
	sysPrxForUser_sys_spu_init();
	sysPrxForUser_sys_game_init();
	sysPrxForUser_sys_libc_init();

	REG_FUNC(sysPrxForUser, sys_initialize_tls);

	REG_FUNC(sysPrxForUser, sys_time_get_system_time);

	// TODO: split syscalls and liblv2 functions
	REG_FUNC(sysPrxForUser, sys_process_exit);
	REG_FUNC(sysPrxForUser, _sys_process_atexitspawn);
	REG_FUNC(sysPrxForUser, _sys_process_at_Exitspawn);
	REG_FUNC(sysPrxForUser, sys_process_is_stack);
	REG_FUNC(sysPrxForUser, sys_process_get_paramsfo); // 0xe75c40f2

	REG_FUNC(sysPrxForUser, sys_interrupt_thread_disestablish);

	REG_FUNC(sysPrxForUser, sys_get_random_number);

	REG_FUNC(sysPrxForUser, console_getc);
	REG_FUNC(sysPrxForUser, console_putc);
	REG_FUNC(sysPrxForUser, console_write);
});
