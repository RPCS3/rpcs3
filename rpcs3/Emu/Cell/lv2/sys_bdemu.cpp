#include "stdafx.h"

#include "Emu/Cell/ErrorCodes.h"

#include "sys_bdemu.h"

LOG_CHANNEL(sys_bdemu);

error_code sys_bdemu_send_command(u64 cmd, u64 a2, u64 a3, vm::ptr<void> buf, u64 buf_len)
{
	sys_bdemu.todo("sys_bdemu_send_command(cmd=0%llx, a2=0x%x, a3=0x%x, buf=0x%x, buf_len=0x%x)", cmd, a2, a3, buf, buf_len);

	return CELL_OK;
}
