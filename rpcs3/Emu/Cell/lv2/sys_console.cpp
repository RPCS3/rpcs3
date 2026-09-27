#include "stdafx.h"
#include "sys_console.h"
#include "Emu/Memory/vm.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/ErrorCodes.h"

#include "sys_console.h"


LOG_CHANNEL(sys_console);

error_code sys_console_write(ppu_thread& ppu, vm::cptr<char> buf, u32 len)
{
	sys_console.todo("sys_console_write(buf=*0x%x, len=0x%x)", buf, len);


	std::string out;
	if (vm::read_string(buf.addr(), len, out, true))
	{
		sys_console.todo("sys_console_write(): \"%s\"", out);

		std::string rr;
		rr = ppu.dump_callstack();
		sys_console.todo("sys_console_write: %s", rr);

	}
	return CELL_OK;
}
