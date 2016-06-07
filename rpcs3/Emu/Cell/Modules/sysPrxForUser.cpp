#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_interrupt.h"
#include "Emu/Cell/lv2/sys_process.h"
#include "sysPrxForUser.h"

logs::channel sysPrxForUser("sysPrxForUser", logs::level::notice);

extern u64 get_system_time();

extern fs::file g_tty;

vm::gvar<s32> sys_prx_version; // ???

#define TLS_SYS 0x30

u32 g_tls_size = 0; // Size of TLS area per thread
u32 g_tls_addr = 0; // Start of TLS memory area
u32 g_tls_max = 0; // Max number of threads

std::unique_ptr<atomic_t<bool>[]> g_tls_map; // I'd like to make it std::vector but it won't work

u32 ppu_alloc_tls()
{
	for (u32 i = 0; i < g_tls_max; i++)
	{
		if (g_tls_map[i].exchange(true) == false)
		{
			const u32 addr = g_tls_addr + i * g_tls_size; // Calculate TLS address
			std::memset(vm::base(addr), 0, TLS_SYS); // Clear system area (TODO)
			std::memcpy(vm::base(addr + TLS_SYS), vm::base(Emu.GetTLSAddr()), Emu.GetTLSFilesz()); // Copy TLS image
			std::memset(vm::base(addr + TLS_SYS + Emu.GetTLSFilesz()), 0, Emu.GetTLSMemsz() - Emu.GetTLSFilesz()); // Clear the rest
			return addr;
		}
	}

	sysPrxForUser.error("ppu_alloc_tls(): out of TLS memory (max=%zu)", g_tls_max);
	return 0;
}

void ppu_free_tls(u32 addr)
{
	// Calculate TLS position
	const u32 i = (addr - g_tls_addr) / g_tls_size;

	if (addr < g_tls_addr || i >= g_tls_max || (addr - g_tls_addr) % g_tls_size)
	{
		sysPrxForUser.error("ppu_free_tls(0x%x): invalid address", addr);
		return;
	}

	if (g_tls_map[i].exchange(false) == false)
	{
		sysPrxForUser.error("ppu_free_tls(0x%x): deallocation failed", addr);
		return;
	}
}

void sys_initialize_tls(PPUThread& ppu, u64 main_thread_id, u32 tls_seg_addr, u32 tls_seg_size, u32 tls_mem_size)
{
	sysPrxForUser.notice("sys_initialize_tls(thread_id=0x%llx, addr=*0x%x, size=0x%x, mem_size=0x%x)", main_thread_id, tls_seg_addr, tls_seg_size, tls_mem_size);

	// Uninitialized TLS expected.
	if (ppu.GPR[13] != 0) return;

	// Initialize TLS memory
	g_tls_size = Emu.GetTLSMemsz() + TLS_SYS;
	g_tls_addr = vm::alloc(0x20000, vm::main) + 0x30;
	g_tls_max = (0xffd0 / g_tls_size) + (0x10000 / g_tls_size);
	g_tls_map = std::make_unique<atomic_t<bool>[]>(g_tls_max);

	// Allocate TLS for main thread
	ppu.GPR[13] = ppu_alloc_tls() + 0x7000 + TLS_SYS;

	sysPrxForUser.notice("TLS initialized (addr=0x%x, size=0x%x, max=0x%x)", g_tls_addr - 0x30, g_tls_size, g_tls_max);

	// TODO
	g_spu_printf_agcb = vm::null;
	g_spu_printf_dgcb = vm::null;
	g_spu_printf_atcb = vm::null;
	g_spu_printf_dtcb = vm::null;
}

s64 sys_time_get_system_time()
{
	sysPrxForUser.trace("sys_time_get_system_time()");

	return get_system_time();
}

s64 _sys_process_atexitspawn()
{
	sysPrxForUser.todo("_sys_process_atexitspawn()");
	return CELL_OK;
}

s64 _sys_process_at_Exitspawn()
{
	sysPrxForUser.todo("_sys_process_at_Exitspawn");
	return CELL_OK;
}

s32 sys_interrupt_thread_disestablish(PPUThread& ppu, u32 ih)
{
	sysPrxForUser.notice("sys_interrupt_thread_disestablish(ih=0x%x)", ih);

	vm::var<u64> r13;

	// Call the syscall
	if (s32 res = _sys_interrupt_thread_disestablish(ppu, ih, r13))
	{
		return res;
	}

	// Deallocate TLS
	ppu_free_tls(vm::cast(*r13, HERE) - 0x7030);

	return CELL_OK;
}

s32 sys_process_is_stack(u32 p)
{
	sysPrxForUser.trace("sys_process_is_stack(p=0x%x)", p);

	// prx: compare high 4 bits with "0xD"
	return (p >> 28) == 0xD;
}

s32 sys_process_get_paramsfo(vm::ptr<char> buffer)
{
	sysPrxForUser.warning("sys_process_get_paramsfo(buffer=*0x%x)", buffer);

	// prx: load some data (0x40 bytes) previously set by _sys_process_get_paramsfo syscall
	return _sys_process_get_paramsfo(buffer);
}

s32 sys_get_random_number(vm::ptr<u8> addr, u64 size)
{
	sysPrxForUser.warning("sys_get_random_number(addr=*0x%x, size=%d)", addr, size);

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

s32 console_write(vm::ptr<char> data, u32 len)
{
	sysPrxForUser.warning("console_write(data=*0x%x, len=%d)", data, len);

	if (g_tty)
	{
		g_tty.write(data.get_ptr(), len);
	}

	return CELL_OK;
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

DECLARE(ppu_module_manager::sysPrxForUser)("sysPrxForUser", []()
{
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

	REG_VAR(sysPrxForUser, sys_prx_version); // 0x7df066cf

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
