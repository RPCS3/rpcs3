#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"

int sys_tty_read(u32 ch, u64 buf_addr, u32 len, u64 preadlen_addr)
{
	ConLog.Warning("sys_tty_read: ch: %d, buf addr: %llx, len: %d", ch, buf_addr, len);
	Memory.Write32NN(preadlen_addr, len);
	Emu.Pause();

	return CELL_OK;
}

int sys_tty_write(u32 ch, u64 buf_addr, u32 len, u64 pwritelen_addr)
{
	if(ch > 15 || (s32)len <= 0) return CELL_EINVAL;
	if(!Memory.IsGoodAddr(buf_addr)) return CELL_EFAULT;
	
	if (!Ini.HLEHideDebugConsole.GetValue())
	{
		Emu.GetDbgCon().Write(ch, Memory.ReadString(buf_addr, len));
	}
	
	if(!Memory.IsGoodAddr(pwritelen_addr)) return CELL_EFAULT;

	Memory.Write32(pwritelen_addr, len);

	return CELL_OK;
}
