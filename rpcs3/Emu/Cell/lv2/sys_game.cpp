#include "stdafx.h"
#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/system_config.h"

#include "sys_game.h"

LOG_CHANNEL(sys_game);

error_code _sys_game_board_storage_read(vm::ptr<u8> buffer, u8 code)
{
	sys_game.trace("sys_game_board_storage_read(buffer=*0x%x, code=0x%02x)", buffer, code);

	if (!buffer)
	{
		return CELL_EFAULT;
	}

	be_t<u64> psid[2] = { +g_cfg.sys.console_psid_high, +g_cfg.sys.console_psid_low };
	u8* psid_bytes = reinterpret_cast<u8*>(psid);

	switch (code)
	{
	case 0xC0:
	case 0xF0:
	{
		u8 response[16] = { 0x01, 0xFC, 0x43, 0x50, 0xA7, 0x9B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
		for (int i = 0; i < 16; i++)
		{
			response[i] ^= psid_bytes[i];
		}
		memcpy(buffer.get_ptr(), response, 16);
		break;
	}
	default:
	{
		sys_game.error("sys_game_board_storage_read(): Unknown code 0x%02x", code);
		break;
	}
	}

	return CELL_OK;
}
