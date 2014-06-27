#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"

int sys_tty_read(u32 ch, u64 buf_addr, u32 len, u64 preadlen_addr)
{
	//we currently do not support reading from the Console
	LOG_WARNING(HLE, "sys_tty_read: ch: %d, buf addr: %llx, len: %d", ch, buf_addr, len);
	Memory.Write32NN(preadlen_addr, len);
	Emu.Pause();

	return CELL_OK;
}

int sys_tty_write(u32 ch, u64 buf_addr, u32 len, u64 pwritelen_addr)
{
	if(ch > 15 || (s32)len <= 0) return CELL_EINVAL;
	if(!Memory.IsGoodAddr(buf_addr)) return CELL_EFAULT;
	
	//ch 0 seems to be stdout and ch 1 stderr
	if (ch == 1)
	{
		LOG_ERROR(TTY, Memory.ReadString(buf_addr, len));
	}
	else
	{
		LOG_NOTICE(TTY, Memory.ReadString(buf_addr, len));
	}
		
	if(!Memory.IsGoodAddr(pwritelen_addr)) return CELL_EFAULT;

	Memory.Write32(pwritelen_addr, len);

	return CELL_OK;
}
