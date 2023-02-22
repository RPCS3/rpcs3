#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/timers.hpp"

#include "Emu/Cell/lv2/sys_mutex.h"
#include "Emu/Cell/lv2/sys_interrupt.h"
#include "Emu/Cell/lv2/sys_process.h"
#include "Emu/Cell/lv2/sys_ss.h"
#include "Emu/Cell/lv2/sys_tty.h"
#include "sysPrxForUser.h"

LOG_CHANNEL(sysPrxForUser);

vm::gvar<s32> sys_prx_version; // ???
vm::gvar<vm::ptr<void()>> g_ppu_atexitspawn;
vm::gvar<vm::ptr<void()>> g_ppu_at_Exitspawn;
extern vm::gvar<u32> g_ppu_exit_mutex;

u64 sys_time_get_system_time()
{
	sysPrxForUser.trace("sys_time_get_system_time()");

	return get_guest_system_time();
}

void sys_process_exit(ppu_thread& ppu, s32 status)
{
	sysPrxForUser.warning("sys_process_exit(status=%d)", status);

	sys_mutex_lock(ppu, *g_ppu_exit_mutex, 0);

	// TODO (process atexit)
	return _sys_process_exit(ppu, status, 0, 0);
}

void _sys_process_atexitspawn(vm::ptr<void()> func)
{
	sysPrxForUser.warning("_sys_process_atexitspawn(0x%x)", func);

	if (!*g_ppu_atexitspawn)
	{
		*g_ppu_atexitspawn = func;
	}
}

void _sys_process_at_Exitspawn(vm::ptr<void()> func)
{
	sysPrxForUser.warning("_sys_process_at_Exitspawn(0x%x)", func);

	if (!*g_ppu_at_Exitspawn)
	{
		*g_ppu_at_Exitspawn = func;
	}
}

s32 sys_process_is_stack(u32 p)
{
	sysPrxForUser.trace("sys_process_is_stack(p=0x%x)", p);

	// prx: compare high 4 bits with "0xD"
	return (p >> 28) == 0xD;
}

error_code sys_process_get_paramsfo(vm::ptr<char> buffer)
{
	sysPrxForUser.warning("sys_process_get_paramsfo(buffer=*0x%x)", buffer);

	// prx: load some data (0x40 bytes) previously set by _sys_process_get_paramsfo syscall
	return _sys_process_get_paramsfo(buffer);
}

error_code sys_get_random_number(vm::ptr<void> addr, u64 size)
{
	sysPrxForUser.warning("sys_get_random_number(addr=*0x%x, size=%d)", addr, size);

	if (size > RANDOM_NUMBER_MAX_SIZE)
	{
		return CELL_EINVAL;
	}

	switch (u32 rs = sys_ss_random_number_generator(2, addr, size))
	{
	case 0x80010501: return CELL_ENOMEM;
	case 0x80010503: return CELL_EAGAIN;
	case 0x80010509: return CELL_EINVAL;
	default: if (rs) return CELL_EABORT;
	}

	return CELL_OK;
}

error_code console_getc()
{
	sysPrxForUser.todo("console_getc()");
	return CELL_OK;
}

void console_putc(ppu_thread& ppu, char ch)
{
	sysPrxForUser.trace("console_putc(ch=0x%x)", ch);
	sys_tty_write(ppu, 0, vm::var<char>(ch), 1, vm::var<u32>{});
}

error_code console_write(ppu_thread& ppu, vm::ptr<char> data, u32 len)
{
	sysPrxForUser.trace("console_write(data=*0x%x, len=%d)", data, len);
	sys_tty_write(ppu, 0, data, len, vm::var<u32>{});
	return CELL_OK;
}

error_code cellGamePs1Emu_61CE2BCD()
{
	UNIMPLEMENTED_FUNC(sysPrxForUser);
	return CELL_OK;
}

error_code cellSysconfPs1emu_639ABBDE()
{
	UNIMPLEMENTED_FUNC(sysPrxForUser);
	return CELL_OK;
}

error_code cellSysconfPs1emu_6A12D11F()
{
	UNIMPLEMENTED_FUNC(sysPrxForUser);
	return CELL_OK;
}

error_code cellSysconfPs1emu_83E79A23()
{
	UNIMPLEMENTED_FUNC(sysPrxForUser);
	return CELL_OK;
}

error_code cellSysconfPs1emu_EFDDAF6C()
{
	UNIMPLEMENTED_FUNC(sysPrxForUser);
	return CELL_OK;
}

error_code sys_lv2coredump_D725F320()
{
	sysPrxForUser.fatal("sys_lv2coredump_D725F320");
	return CELL_OK;
}

error_code sys_get_bd_media_id()
{
	UNIMPLEMENTED_FUNC(sysPrxForUser);
	return CELL_OK;
}

error_code sys_get_console_id()
{
	UNIMPLEMENTED_FUNC(sysPrxForUser);
	return CELL_OK;
}

error_code sysPs2Disc_A84FD3C3()
{
	UNIMPLEMENTED_FUNC(sysPrxForUser);
	return CELL_OK;
}

error_code sysPs2Disc_BB7CD1AE()
{
	UNIMPLEMENTED_FUNC(sysPrxForUser);
	return CELL_OK;
}

extern void sysPrxForUser_sys_lwmutex_init(ppu_static_module*);
extern void sysPrxForUser_sys_lwcond_init(ppu_static_module*);
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

DECLARE(ppu_module_manager::sysPrxForUser)("sysPrxForUser", [](ppu_static_module* _this)
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

	sysPrxForUser_sys_lwmutex_init(_this);
	sysPrxForUser_sys_lwcond_init(_this);
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
	REG_VAR(sysPrxForUser, g_ppu_atexitspawn).flag(MFF_HIDDEN);
	REG_VAR(sysPrxForUser, g_ppu_at_Exitspawn).flag(MFF_HIDDEN);

	REG_FUNC(sysPrxForUser, sys_time_get_system_time);

	REG_FUNC(sysPrxForUser, sys_process_exit);
	REG_FUNC(sysPrxForUser, _sys_process_atexitspawn);
	REG_FUNC(sysPrxForUser, _sys_process_at_Exitspawn);
	REG_FUNC(sysPrxForUser, sys_process_is_stack);
	REG_FUNC(sysPrxForUser, sys_process_get_paramsfo); // 0xe75c40f2

	REG_FUNC(sysPrxForUser, sys_get_random_number);

	REG_FUNC(sysPrxForUser, console_getc);
	REG_FUNC(sysPrxForUser, console_putc);
	REG_FUNC(sysPrxForUser, console_write);
});
