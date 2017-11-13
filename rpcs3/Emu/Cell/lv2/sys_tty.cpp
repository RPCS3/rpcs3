#include "stdafx.h"
#include "sys_tty.h"

namespace vm { using namespace ps3; }

logs::channel sys_tty("sys_tty");

extern fs::file g_tty;

error_code sys_tty_read(s32 ch, vm::ptr<char> buf, u32 len, vm::ptr<u32> preadlen)
{
	sys_tty.todo("sys_tty_read(ch=%d, buf=*0x%x, len=%d, preadlen=*0x%x)", ch, buf, len, preadlen);
	std::string input = "insert string here";
	if (input.length() > len)
	{
		*preadlen = len;
		std::strncpy(buf.get_ptr(), input.c_str(), len);
	}
	else
	{
		*preadlen = input.length();
		std::strcpy(buf.get_ptr(), input.c_str());
	}
	return CELL_OK;
}

error_code sys_tty_write(s32 ch, vm::cptr<char> buf, u32 len, vm::ptr<u32> pwritelen)
{
	sys_tty.notice("sys_tty_write(ch=%d, buf=*0x%x, len=%d, pwritelen=*0x%x)", ch, buf, len, pwritelen);

	if (ch > 15)
	{
		return CELL_EINVAL;
	}

	const u32 written_len = static_cast<s32>(len) > 0 ? len : 0;

	if (written_len > 0 && g_tty)
	{
		g_tty.write(buf.get_ptr(), len);
	}

	if (!pwritelen)
	{
		return CELL_EFAULT;
	}

	*pwritelen = written_len;

	return CELL_OK;
}
