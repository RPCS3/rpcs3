#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "Emu/Cell/ErrorCodes.h"
#include "sys_tty.h"

logs::channel sys_tty("sys_tty", logs::level::notice);

extern fs::file g_tty;

s32 sys_tty_read(s32 ch, vm::ptr<char> buf, u32 len, vm::ptr<u32> preadlen)
{
	sys_tty.todo("sys_tty_read(ch=%d, buf=*0x%x, len=%d, preadlen=*0x%x)", ch, buf, len, preadlen);

	// We currently do not support reading from the Console
	*preadlen = 0;
	Emu.Pause();
	return CELL_OK;
}

s32 sys_tty_write(s32 ch, vm::cptr<char> buf, u32 len, vm::ptr<u32> pwritelen)
{
	sys_tty.notice("sys_tty_write(ch=%d, buf=*0x%x, len=%d, pwritelen=*0x%x)", ch, buf, len, pwritelen);

	if (ch > 15)
	{
		return CELL_EINVAL;
	}

	if ((s32)len <= 0)
	{
		*pwritelen = 0;

		return CELL_OK;
	}

	if (g_tty)
	{
		g_tty.write(buf.get_ptr(), len);
	}

	*pwritelen = len;

	return CELL_OK;
}
