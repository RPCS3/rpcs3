#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"

// TTY channels
enum
{
	SYS_TTYP_PPU_STDIN   = 0,
	SYS_TTYP_PPU_STDOUT  = 0,
	SYS_TTYP_PPU_STDERR  = 1,
	SYS_TTYP_SPU_STDOUT  = 2,
	SYS_TTYP_USER1       = 3,
	SYS_TTYP_USER2       = 4,
	SYS_TTYP_USER3       = 5,
	SYS_TTYP_USER4       = 6,
	SYS_TTYP_USER5       = 7,
	SYS_TTYP_USER6       = 8,
	SYS_TTYP_USER7       = 9,
	SYS_TTYP_USER8       = 10,
	SYS_TTYP_USER9       = 11,
	SYS_TTYP_USER10      = 12,
	SYS_TTYP_USER11      = 13,
	SYS_TTYP_USER12      = 14,
	SYS_TTYP_USER13      = 15,
};

class ppu_thread;

// SysCalls
error_code sys_tty_read(s32 ch, vm::ptr<char> buf, u32 len, vm::ptr<u32> preadlen);
error_code sys_tty_write(ppu_thread& ppu, s32 ch, vm::cptr<char> buf, u32 len, vm::ptr<u32> pwritelen);
