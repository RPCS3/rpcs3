#include "stdafx.h"
#include "util/sysinfo.hpp"
#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/System.h"
#include "Emu/system_config.h"

#include <thread>

#include "sys_game.h"

LOG_CHANNEL(sys_game);

atomic_t<bool> watchdog_stopped = true;
atomic_t<u64> watchdog_last_clear;
u64 get_timestamp()
{
	return (get_system_time() - Emu.GetPauseTime());
}

error_code _sys_game_watchdog_start(u32 timeout)
{
	sys_game.trace("sys_game_watchdog_start(timeout=%d)", timeout);

	if (!watchdog_stopped)
	{
		return CELL_EABORT;
	}

	auto watchdog = [=]()
	{
		while (!watchdog_stopped)
		{
			if (Emu.IsStopped() || get_timestamp() - watchdog_last_clear > timeout * 1000000)
			{
				watchdog_stopped = true;
				if (!Emu.IsStopped())
				{
					sys_game.warning("Watchdog timeout! Restarting the game...");
					Emu.CallFromMainThread([]()
						{
							Emu.Restart();
						});
				}
				break;
			}
			std::this_thread::sleep_for(1s);
		}
	};

	watchdog_stopped = false;
	watchdog_last_clear = get_timestamp();
	std::thread(watchdog).detach();

	return CELL_OK;
}

error_code _sys_game_watchdog_stop()
{
	sys_game.trace("sys_game_watchdog_stop()");

	watchdog_stopped = true;

	return CELL_OK;
}

error_code _sys_game_watchdog_clear()
{
	sys_game.trace("sys_game_watchdog_clear()");

	watchdog_last_clear = get_timestamp();

	return CELL_OK;
}

u64 _sys_game_get_system_sw_version()
{
	return stof(utils::get_firmware_version()) * 10000;
}

error_code _sys_game_board_storage_read(vm::ptr<u8> buffer1, vm::ptr<u8> buffer2)
{
	sys_game.trace("sys_game_board_storage_read(buffer1=*0x%x, buffer2=*0x%x)", buffer1, buffer2);

	if (!buffer1)
	{
		return CELL_EFAULT;
	}

	be_t<u64> psid[2] = { +g_cfg.sys.console_psid_high, +g_cfg.sys.console_psid_low };
	u8* psid_bytes = reinterpret_cast<u8*>(psid);
	u8 response[16] = { 0x01, 0xFC, 0x43, 0x50, 0xA7, 0x9B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	for (int i = 0; i < 16; i++)
	{
		response[i] ^= psid_bytes[i];
	}
	memcpy(buffer1.get_ptr(), response, 16);

	if (!buffer2)
	{
		return CELL_EFAULT;
	}

	*buffer2 = 0x00;

	return CELL_OK;
}
