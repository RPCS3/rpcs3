#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"
#include "sys_tty.h"

s32 sys_tty_read(u32 ch, u64 buf_addr, u32 len, u64 preadlen_addr)
{
	// We currently do not support reading from the Console
	LOG_WARNING(HLE, "sys_tty_read: ch: %d, buf addr: %llx, len: %d", ch, buf_addr, len);
	Memory.Write32(preadlen_addr, len);
	Emu.Pause();

	return CELL_OK;
}

s32 sys_tty_write(u32 ch, u64 buf_addr, u32 len, u64 pwritelen_addr)
{
	if(ch > 15 || (s32)len <= 0) return CELL_EINVAL;
	if(!Memory.IsGoodAddr(buf_addr)) return CELL_EFAULT;
	
	if (ch == SYS_TTYP_PPU_STDOUT || ch == SYS_TTYP_SPU_STDOUT || (ch >= SYS_TTYP_USER1 && ch <= SYS_TTYP_USER13)) {
		LOG_NOTICE(TTY, Memory.ReadString(buf_addr, len));
	}
	if (ch == SYS_TTYP_PPU_STDERR) {
		LOG_ERROR(TTY, Memory.ReadString(buf_addr, len));
	}
		
	if(!Memory.IsGoodAddr(pwritelen_addr)) return CELL_EFAULT;

	Memory.Write32(pwritelen_addr, len);

	return CELL_OK;
}
