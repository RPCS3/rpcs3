#include "stdafx.h"
#include "sys_tty.h"

namespace vm { using namespace ps3; }

logs::channel sys_tty("sys_tty");

extern fs::file g_tty;

error_code sys_tty_read(s32 ch, vm::ptr<char> buf, u32 len, vm::ptr<u32> preadlen)
{
	sys_tty.fatal("sys_tty_read(ch=%d, buf=*0x%x, len=%d, preadlen=*0x%x)", ch, buf, len, preadlen);

	// We currently do not support reading from the Console
	fmt::throw_exception("Unimplemented" HERE);
}

error_code sys_tty_write(s32 ch, vm::cptr<char> buf, u32 len, vm::ptr<u32> pwritelen)
{
	sys_tty.notice("sys_tty_write(ch=%d, buf=*0x%x, len=%d, pwritelen=*0x%x)", ch, buf, len, pwritelen);

	if (ch > 15)
	{
		return CELL_EINVAL;
	}

	if (static_cast<s32>(len) <= 0)
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
