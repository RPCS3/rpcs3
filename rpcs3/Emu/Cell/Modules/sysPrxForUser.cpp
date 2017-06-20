#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_interrupt.h"
#include "Emu/Cell/lv2/sys_process.h"
#include "sysPrxForUser.h"

logs::channel sysPrxForUser("sysPrxForUser");

extern u64 get_system_time();

extern fs::file g_tty;

vm::gvar<s32> sys_prx_version; // ???

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
extern void sysPrxForUser_sys_rsxaudio_init();

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
	sysPrxForUser_sys_rsxaudio_init();

	REG_VAR(sysPrxForUser, sys_prx_version); // 0x7df066cf

	REG_FUNC(sysPrxForUser, sys_time_get_system_time);

	// TODO: split syscalls and liblv2 functions
	REG_FUNC(sysPrxForUser, sys_process_exit);
	REG_FUNC(sysPrxForUser, _sys_process_atexitspawn);
	REG_FUNC(sysPrxForUser, _sys_process_at_Exitspawn);
	REG_FUNC(sysPrxForUser, sys_process_is_stack);
	REG_FUNC(sysPrxForUser, sys_process_get_paramsfo); // 0xe75c40f2

	REG_FUNC(sysPrxForUser, sys_get_random_number);

	REG_FUNC(sysPrxForUser, __sys_look_ctype_table);

	REG_FUNC(sysPrxForUser, console_getc);
	REG_FUNC(sysPrxForUser, console_putc);
	REG_FUNC(sysPrxForUser, console_write);
});
