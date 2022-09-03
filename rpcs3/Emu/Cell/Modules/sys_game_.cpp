#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_mutex.h"
#include "Emu/Cell/lv2/sys_process.h"
#include "sysPrxForUser.h"

LOG_CHANNEL(sysPrxForUser);

extern vm::gvar<u32> g_ppu_exit_mutex;
extern vm::gvar<vm::ptr<void()>> g_ppu_atexitspawn;
extern vm::gvar<vm::ptr<void()>> g_ppu_at_Exitspawn;

static u32 get_string_array_size(vm::cpptr<char> list, u32& out_count)
{
	//out_count = 0;
	u32 result = 8;

	for (u32 i = 0; list; i++)
	{
		if (const vm::cptr<char> str = list[i])
		{
			out_count++;
			result += ((static_cast<u32>(std::strlen(str.get_ptr())) + 0x10) & -0x10) + 8;
			continue;
		}
		break;
	}

	return result;
}

static u32 get_exitspawn_size(vm::cptr<char> path, vm::cpptr<char> argv, vm::cpptr<char> envp, u32& arg_count, u32& env_count)
{
	arg_count = 1;
	env_count = 0;

	u32 result = ((static_cast<u32>(std::strlen(path.get_ptr())) + 0x10) & -0x10) + 8;
	result += get_string_array_size(argv, arg_count);
	result += get_string_array_size(envp, env_count);

	if ((arg_count + env_count) % 2)
	{
		result += 8;
	}

	return result;
}

static void put_string_array(vm::pptr<char, u32, u64> pstr, vm::ptr<char>& str, u32 count, vm::cpptr<char> list)
{
	for (u32 i = 0; i < count; i++)
	{
		const u32 len = static_cast<u32>(std::strlen(list[i].get_ptr()));
		std::memcpy(str.get_ptr(), list[i].get_ptr(), len + 1);
		pstr[i] = str;
		str += (len + 0x10) & -0x10;
	}

	pstr[count] = vm::null;
}

static void put_exitspawn(vm::ptr<void> out, vm::cptr<char> path, u32 argc, vm::cpptr<char> argv, u32 envc, vm::cpptr<char> envp)
{
	vm::pptr<char, u32, u64> pstr = vm::cast(out.addr());
	vm::ptr<char> str = vm::static_ptr_cast<char>(out) + (argc + envc + (argc + envc) % 2) * 8 + 0x10;

	const u32 len = static_cast<u32>(std::strlen(path.get_ptr()));
	std::memcpy(str.get_ptr(), path.get_ptr(), len + 1);
	*pstr++ = str;
	str += (len + 0x10) & -0x10;

	put_string_array(pstr, str, argc - 1, argv);
	put_string_array(pstr + argc, str, envc, envp);
}

static void exitspawn(ppu_thread& ppu, vm::cptr<char> path, vm::cpptr<char> argv, vm::cpptr<char> envp, u32 data, u32 data_size, s32 prio, u64 _flags)
{
	sys_mutex_lock(ppu, *g_ppu_exit_mutex, 0);

	u32 arg_count = 0;
	u32 env_count = 0;
	u32 alloc_size = get_exitspawn_size(path, argv, envp, arg_count, env_count);

	if (alloc_size > 0x1000)
	{
		argv = vm::null;
		envp = vm::null;
		arg_count = 0;
		env_count = 0;
		alloc_size = get_exitspawn_size(path, vm::null, vm::null, arg_count, env_count);
	}

	alloc_size += 0x30;

	if (data_size > 0)
	{
		alloc_size += 0x1030;
	}

	u32 alloc_addr = vm::alloc(alloc_size, vm::main);

	if (!alloc_addr)
	{
		// TODO (process atexit)
		return _sys_process_exit(ppu, CELL_ENOMEM, 0, 0);
	}

	put_exitspawn(vm::cast(alloc_addr + 0x30), path, arg_count, argv, env_count, envp);

	if (data_size)
	{
		std::memcpy(vm::base(alloc_addr + alloc_size - 0x1000), vm::base(data), std::min<u32>(data_size, 0x1000));
	}

	vm::ptr<sys_exit2_param> arg = vm::cast(alloc_addr);
	arg->x0        = 0x85;
	arg->this_size = 0x30;
	arg->next_size = alloc_size - 0x30;
	arg->prio      = prio;
	arg->flags     = _flags;
	arg->args      = vm::cast(alloc_addr + 0x30);

	if ((_flags >> 62) == 0 && *g_ppu_atexitspawn)
	{
		// Execute atexitspawn
		g_ppu_atexitspawn->operator()(ppu);
	}

	if ((_flags >> 62) == 1 && *g_ppu_at_Exitspawn)
	{
		// Execute at_Exitspawn
		g_ppu_at_Exitspawn->operator()(ppu);
	}

	// TODO (process atexit)
	return _sys_process_exit2(ppu, 0, arg, alloc_size, 0x10000000);
}

void sys_game_process_exitspawn(ppu_thread& ppu, vm::cptr<char> path, vm::cpptr<char> argv, vm::cpptr<char> envp, u32 data, u32 data_size, s32 prio, u64 flags)
{
	sysPrxForUser.warning("sys_game_process_exitspawn(path=%s, argv=**0x%x, envp=**0x%x, data=*0x%x, data_size=0x%x, prio=%d, flags=0x%x)", path, argv, envp, data, data_size, prio, flags);

	return exitspawn(ppu, path, argv, envp, data, data_size, prio, (flags & 0xf0) | (1ull << 63));
}

void sys_game_process_exitspawn2(ppu_thread& ppu, vm::cptr<char> path, vm::cpptr<char> argv, vm::cpptr<char> envp, u32 data, u32 data_size, s32 prio, u64 flags)
{
	sysPrxForUser.warning("sys_game_process_exitspawn2(path=%s, argv=**0x%x, envp=**0x%x, data=*0x%x, data_size=0x%x, prio=%d, flags=0x%x)", path, argv, envp, data, data_size, prio, flags);

	return exitspawn(ppu, path, argv, envp, data, data_size, prio, (flags >> 62) >= 2 ? flags & 0xf0 : flags & 0xc0000000000000f0ull);
}

error_code sys_game_board_storage_read()
{
	sysPrxForUser.todo("sys_game_board_storage_read()");
	return CELL_OK;
}

error_code sys_game_board_storage_write()
{
	sysPrxForUser.todo("sys_game_board_storage_write()");
	return CELL_OK;
}

error_code sys_game_get_rtc_status()
{
	sysPrxForUser.todo("sys_game_get_rtc_status()");
	return CELL_OK;
}

error_code sys_game_get_system_sw_version()
{
	sysPrxForUser.todo("sys_game_get_system_sw_version()");
	return CELL_OK;
}

error_code sys_game_get_temperature()
{
	sysPrxForUser.todo("sys_game_get_temperature()");
	return CELL_OK;
}

error_code sys_game_watchdog_clear()
{
	sysPrxForUser.todo("sys_game_watchdog_clear()");
	return CELL_OK;
}

error_code sys_game_watchdog_start()
{
	sysPrxForUser.todo("sys_game_watchdog_start()");
	return CELL_OK;
}

error_code sys_game_watchdog_stop()
{
	sysPrxForUser.todo("sys_game_watchdog_stop()");
	return CELL_OK;
}


void sysPrxForUser_sys_game_init()
{
	REG_FUNC(sysPrxForUser, sys_game_process_exitspawn2);
	REG_FUNC(sysPrxForUser, sys_game_process_exitspawn);
	REG_FUNC(sysPrxForUser, sys_game_board_storage_read);
	REG_FUNC(sysPrxForUser, sys_game_board_storage_write);
	REG_FUNC(sysPrxForUser, sys_game_get_rtc_status);
	REG_FUNC(sysPrxForUser, sys_game_get_system_sw_version);
	REG_FUNC(sysPrxForUser, sys_game_get_temperature);
	REG_FUNC(sysPrxForUser, sys_game_watchdog_clear);
	REG_FUNC(sysPrxForUser, sys_game_watchdog_start);
	REG_FUNC(sysPrxForUser, sys_game_watchdog_stop);
}
