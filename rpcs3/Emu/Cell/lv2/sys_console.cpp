#include "stdafx.h"
#include "Emu/Cell/ErrorCodes.h"

#include "Emu/Memory/vm.h"
#include "sys_tty.h"
#include "sys_console.h"


LOG_CHANNEL(sys_console);

error_code sys_console_write(vm::cptr<char> buf, u32 len)
{
	sys_console.todo("sys_console_write(buf=%s, len=0x%x)", buf, len);

	// to make this easier to spot, also piping to tty
	std::string tmp(buf.get_ptr(), len);
	tmp = "CONSOLE: " + tmp;
	sys_tty_write(0, vm::make_str(tmp), tmp.size(), vm::var<u32>{});

	return CELL_OK;
}
