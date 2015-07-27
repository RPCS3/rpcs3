#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"

#include "sys_tty.h"

SysCallBase sys_tty("sys_tty");

s32 sys_tty_read(s32 ch, vm::ptr<char> buf, u32 len, vm::ptr<u32> preadlen)
{
	sys_tty.Error("sys_tty_read(ch=%d, buf=*0x%x, len=%d, preadlen=*0x%x)", ch, buf, len, preadlen);

	// We currently do not support reading from the Console
	*preadlen = 0;
	Emu.Pause();
	return CELL_OK;
}

s32 sys_tty_write(s32 ch, vm::cptr<char> buf, u32 len, vm::ptr<u32> pwritelen)
{
	sys_tty.Log("sys_tty_write(ch=%d, buf=*0x%x, len=%d, pwritelen=*0x%x)", ch, buf, len, pwritelen);

	if (ch > 15)
	{
		return CELL_EINVAL;
	}

	if ((s32)len <= 0)
	{
		*pwritelen = 0;

		return CELL_OK;
	}

	log_message(Log::LogType::TTY, ch == SYS_TTYP_PPU_STDERR ? Log::Severity::Error : Log::Severity::Notice, { buf.get_ptr(), len });
		
	*pwritelen = len;

	return CELL_OK;
}
