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

static u32 s_tls_addr = 0; // TLS image address
static u32 s_tls_file = 0; // TLS image size
static u32 s_tls_zero = 0; // TLS zeroed area size (TLS mem size - TLS image size)
static u32 s_tls_size = 0; // Size of TLS area per thread
static u32 s_tls_area = 0; // Start of TLS memory area
static u32 s_tls_max = 0; // Max number of threads
static std::unique_ptr<atomic_t<bool>[]> s_tls_map; // I'd like to make it std::vector but it won't work

u32 ppu_alloc_tls()
{
	u32 addr = 0;

	for (u32 i = 0; i < s_tls_max; i++)
	{
		if (!s_tls_map[i] && s_tls_map[i].exchange(true) == false)
		{
			// Default (small) TLS allocation
			addr = s_tls_area + i * s_tls_size;
			break;			
		}
	}

	if (!addr)
	{
		// Alternative (big) TLS allocation
		addr = vm::alloc(s_tls_size, vm::main);
	}

	std::memset(vm::base(addr), 0, 0x30); // Clear system area (TODO)
	std::memcpy(vm::base(addr + 0x30), vm::base(s_tls_addr), s_tls_file); // Copy TLS image
	std::memset(vm::base(addr + 0x30 + s_tls_file), 0, s_tls_zero); // Clear the rest
	return addr;
}

void ppu_free_tls(u32 addr)
{
	// Calculate TLS position
	const u32 i = (addr - s_tls_area) / s_tls_size;

	if (addr < s_tls_area || i >= s_tls_max || (addr - s_tls_area) % s_tls_size)
	{
		// Alternative TLS allocation detected
		vm::dealloc_verbose_nothrow(addr, vm::main);
		return;
	}

	if (s_tls_map[i].exchange(false) == false)
	{
		sysPrxForUser.error("ppu_free_tls(0x%x): deallocation failed", addr);
		return;
	}
}

void sys_initialize_tls(ppu_thread& ppu, u64 main_thread_id, u32 tls_seg_addr, u32 tls_seg_size, u32 tls_mem_size)
{
	sysPrxForUser.notice("sys_initialize_tls(thread_id=0x%llx, addr=*0x%x, size=0x%x, mem_size=0x%x)", main_thread_id, tls_seg_addr, tls_seg_size, tls_mem_size);

	// Uninitialized TLS expected.
	if (ppu.gpr[13] != 0) return;

	// Initialize TLS memory
	s_tls_addr = tls_seg_addr;
	s_tls_file = tls_seg_size;
	s_tls_zero = tls_mem_size - tls_seg_size;
	s_tls_size = tls_mem_size + 0x30; // 0x30 is system area size
	s_tls_area = vm::alloc(0x40000, vm::main) + 0x30;
	s_tls_max = (0x40000 - 0x30) / s_tls_size;
	s_tls_map = std::make_unique<atomic_t<bool>[]>(s_tls_max);

	// Allocate TLS for main thread
	ppu.gpr[13] = ppu_alloc_tls() + 0x7000 + 0x30;

	sysPrxForUser.notice("TLS initialized (addr=0x%x, size=0x%x, max=0x%x)", s_tls_area - 0x30, s_tls_size, s_tls_max);

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

s32 sys_interrupt_thread_disestablish(ppu_thread& ppu, u32 ih)
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

s32 __sys_look_ctype_table()
{
	UNIMPLEMENTED_FUNC(sysPrxForUser);
	return CELL_OK;
}

s32 console_getc()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 console_putc()
{
	fmt::throw_exception("Unimplemented" HERE);
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

s32 cellGamePs1Emu_61CE2BCD()
{
	UNIMPLEMENTED_FUNC(logs::HLE);
	return CELL_OK;
}

s32 cellSysconfPs1emu_639ABBDE()
{
	UNIMPLEMENTED_FUNC(logs::HLE);
	return CELL_OK;
}

s32 cellSysconfPs1emu_6A12D11F()
{
	UNIMPLEMENTED_FUNC(logs::HLE);
	return CELL_OK;
}

s32 cellSysconfPs1emu_83E79A23()
{
	UNIMPLEMENTED_FUNC(logs::HLE);
	return CELL_OK;
}

s32 cellSysconfPs1emu_EFDDAF6C()
{
	UNIMPLEMENTED_FUNC(logs::HLE);
	return CELL_OK;
}

s32 sys_lv2coredump_D725F320()
{
	fmt::raw_error(__func__);
}

s32 sys_crash_dump_get_user_log_area()
{
	fmt::raw_error(__func__);
}

s32 sys_crash_dump_set_user_log_area()
{
	UNIMPLEMENTED_FUNC(logs::HLE);
	return CELL_OK;
}

s32 sys_get_bd_media_id()
{
	UNIMPLEMENTED_FUNC(logs::HLE);
	return CELL_OK;
}

s32 sys_get_console_id()
{
	UNIMPLEMENTED_FUNC(logs::HLE);
	return CELL_OK;
}

s32 sysPs2Disc_A84FD3C3()
{
	UNIMPLEMENTED_FUNC(logs::HLE);
	return CELL_OK;
}

s32 sysPs2Disc_BB7CD1AE()
{
	UNIMPLEMENTED_FUNC(logs::HLE);
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
	static ppu_static_module cellGamePs1Emu("cellGamePs1Emu", []()
	{
		REG_FNID(cellGamePs1Emu, 0x61CE2BCD, cellGamePs1Emu_61CE2BCD);
	});

	static ppu_static_module cellSysconfPs1emu("cellSysconfPs1emu", []()
	{
		REG_FNID(cellSysconfPs1emu, 0x639ABBDE, cellSysconfPs1emu_639ABBDE);
		REG_FNID(cellSysconfPs1emu, 0x6A12D11F, cellSysconfPs1emu_6A12D11F);
		REG_FNID(cellSysconfPs1emu, 0x83E79A23, cellSysconfPs1emu_83E79A23);
		REG_FNID(cellSysconfPs1emu, 0xEFDDAF6C, cellSysconfPs1emu_EFDDAF6C);
	});

	static ppu_static_module sys_lv2coredump("sys_lv2coredump", []()
	{
		REG_FNID(sys_lv2coredump, 0xD725F320, sys_lv2coredump_D725F320);
	});

	static ppu_static_module sys_crashdump("sys_crashdump", []()
	{
		REG_FUNC(sys_crashdump, sys_crash_dump_get_user_log_area);
		REG_FUNC(sys_crashdump, sys_crash_dump_set_user_log_area);
	});

	static ppu_static_module sysBdMediaId("sysBdMediaId", []()
	{
		REG_FUNC(sysBdMediaId, sys_get_bd_media_id);
	});

	static ppu_static_module sysConsoleId("sysConsoleId", []()
	{
		REG_FUNC(sysConsoleId, sys_get_console_id);
	});

	static ppu_static_module sysPs2Disc("sysPs2Disc", []()
	{
		REG_FNID(sysPs2Disc, 0xA84FD3C3, sysPs2Disc_A84FD3C3);
		REG_FNID(sysPs2Disc, 0xBB7CD1AE, sysPs2Disc_BB7CD1AE);
	});

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

	REG_FUNC(sysPrxForUser, __sys_look_ctype_table);

	REG_FUNC(sysPrxForUser, console_getc);
	REG_FUNC(sysPrxForUser, console_putc);
	REG_FUNC(sysPrxForUser, console_write);
});
