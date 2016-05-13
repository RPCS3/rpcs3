#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "sysPrxForUser.h"

extern logs::channel sysPrxForUser;

void sys_game_process_exitspawn(vm::cptr<char> path, u32 argv_addr, u32 envp_addr, u32 data_addr, u32 data_size, u32 prio, u64 flags)
{
	std::string _path = path.get_ptr();
	const std::string& from = "//";
	const std::string& to = "/";

	size_t start_pos = 0;
	while ((start_pos = _path.find(from, start_pos)) != std::string::npos) {
		_path.replace(start_pos, from.length(), to);
		start_pos += to.length();
	}

	sysPrxForUser.todo("sys_game_process_exitspawn()");
	sysPrxForUser.warning("path: %s", _path.c_str());
	sysPrxForUser.warning("argv: 0x%x", argv_addr);
	sysPrxForUser.warning("envp: 0x%x", envp_addr);
	sysPrxForUser.warning("data: 0x%x", data_addr);
	sysPrxForUser.warning("data_size: 0x%x", data_size);
	sysPrxForUser.warning("prio: %d", prio);
	sysPrxForUser.warning("flags: %d", flags);

	std::vector<std::string> argv;
	std::vector<std::string> env;

	if (argv_addr)
	{
		auto argvp = vm::cpptr<char>::make(argv_addr);
		while (argvp && *argvp)
		{
			argv.push_back(argvp[0].get_ptr());
			argvp++;
		}

		for (auto &arg : argv)
		{
			sysPrxForUser.trace("argument: %s", arg.c_str());
		}
	}

	if (envp_addr)
	{
		auto envp = vm::cpptr<char>::make(envp_addr);
		while (envp && *envp)
		{
			env.push_back(envp[0].get_ptr());
			envp++;
		}

		for (auto &en : env)
		{
			sysPrxForUser.trace("env_argument: %s", en.c_str());
		}
	}

	// TODO: execute the file in <path> with the args in argv
	// and the environment parameters in envp and copy the data
	// from data_addr into the adress space of the new process
	// then kill the current process

	Emu.Pause();
	sysPrxForUser.success("Process finished");

	Emu.CallAfter([=, path = vfs::get(_path)]()
	{
		Emu.Stop();
		Emu.BootGame(path, true);
	});
}

void sys_game_process_exitspawn2(vm::cptr<char> path, u32 argv_addr, u32 envp_addr, u32 data_addr, u32 data_size, u32 prio, u64 flags)
{
	std::string _path = path.get_ptr();
	const std::string& from = "//";
	const std::string& to = "/";

	size_t start_pos = 0;
	while ((start_pos = _path.find(from, start_pos)) != std::string::npos) {
		_path.replace(start_pos, from.length(), to);
		start_pos += to.length();
	}

	sysPrxForUser.warning("sys_game_process_exitspawn2()");
	sysPrxForUser.warning("path: %s", _path.c_str());
	sysPrxForUser.warning("argv: 0x%x", argv_addr);
	sysPrxForUser.warning("envp: 0x%x", envp_addr);
	sysPrxForUser.warning("data: 0x%x", data_addr);
	sysPrxForUser.warning("data_size: 0x%x", data_size);
	sysPrxForUser.warning("prio: %d", prio);
	sysPrxForUser.warning("flags: %d", flags);

	std::vector<std::string> argv;
	std::vector<std::string> env;

	if (argv_addr)
	{
		auto argvp = vm::cpptr<char>::make(argv_addr);
		while (argvp && *argvp)
		{
			argv.push_back(argvp[0].get_ptr());
			argvp++;
		}

		for (auto &arg : argv)
		{
			sysPrxForUser.trace("argument: %s", arg.c_str());
		}
	}

	if (envp_addr)
	{
		auto envp = vm::cpptr<char>::make(envp_addr);
		while (envp && *envp)
		{
			env.push_back(envp[0].get_ptr());
			envp++;
		}

		for (auto &en : env)
		{
			sysPrxForUser.trace("env_argument: %s", en.c_str());
		}
	}

	// TODO: execute the file in <path> with the args in argv
	// and the environment parameters in envp and copy the data
	// from data_addr into the adress space of the new process
	// then kill the current process

	Emu.Pause();
	sysPrxForUser.success("Process finished");

	Emu.CallAfter([=, path = vfs::get(_path)]()
	{
		Emu.Stop();
		Emu.BootGame(path, true);
	});

	return;
}

s32 sys_game_board_storage_read()
{
	throw EXCEPTION("");
}

s32 sys_game_board_storage_write()
{
	throw EXCEPTION("");
}

s32 sys_game_get_rtc_status()
{
	throw EXCEPTION("");
}

s32 sys_game_get_system_sw_version()
{
	throw EXCEPTION("");
}

s32 sys_game_get_temperature()
{
	throw EXCEPTION("");
}

s32 sys_game_watchdog_clear()
{
	throw EXCEPTION("");
}

s32 sys_game_watchdog_start()
{
	throw EXCEPTION("");
}

s32 sys_game_watchdog_stop()
{
	throw EXCEPTION("");
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
