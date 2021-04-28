#include "stdafx.h"
#include "Emu/Cell/ErrorCodes.h"

#include "sys_console.h"


LOG_CHANNEL(sys_console);

error_code sys_console_write(vm::cptr<char> buf, u32 len)
{
	sys_console.todo("sys_console_write(buf=*0x%x, len=0x%x)", buf, len);

	return CELL_OK;
}
