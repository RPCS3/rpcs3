#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"

#include "sys_tty.h"

SysCallBase sys_tty("sys_tty");

s32 sys_tty_read(s32 ch, vm::ptr<void> buf, u32 len, vm::ptr<u32> preadlen)
{
	sys_tty.Error("sys_tty_read(ch=%d, buf_addr=0x%x, len=%d, preadlen_addr=0x%x)", ch, buf.addr(), len, preadlen.addr());

	// We currently do not support reading from the Console
	*preadlen = 0;
	Emu.Pause();
	return CELL_OK;
}

s32 sys_tty_write(s32 ch, vm::cptr<void> buf, u32 len, vm::ptr<u32> pwritelen)
{
	sys_tty.Log("sys_tty_write(ch=%d, buf_addr=0x%x, len=%d, preadlen_addr=0x%x)", ch, buf.addr(), len, pwritelen.addr());

	if (ch > 15)
	{
		sys_tty.Error("sys_tty_write(): specified channel was higher than 15.");
		return CELL_EINVAL;
	}

	if ((s32) len <= 0)
	{
		sys_tty.Error("sys_tty_write(): specified length was 0.");
		return CELL_OK;
	}

	const std::string data((const char*)buf.get_ptr(), len);
	
	if (ch == SYS_TTYP_PPU_STDOUT || ch == SYS_TTYP_SPU_STDOUT || (ch >= SYS_TTYP_USER1 && ch <= SYS_TTYP_USER13)) {
		LOG_NOTICE(TTY, "%s", data.c_str());
	}
	if (ch == SYS_TTYP_PPU_STDERR) {
		LOG_ERROR(TTY, "%s", data.c_str());
	}
		
	*pwritelen = (u32)data.size();
	return CELL_OK;
}
