#include "stdafx.h"
#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/system_config.h"

#include "sys_game.h"

LOG_CHANNEL(sys_game);

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
