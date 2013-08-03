#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void sys_io_init();
Module sys_io(0x0017, sys_io_init);

void sys_io_init()
{
	sys_io.AddFunc(0x1cf98800, cellPadInit);
	sys_io.AddFunc(0x4d9b75d5, cellPadEnd);
	sys_io.AddFunc(0x0d5f2c14, cellPadClearBuf);
	sys_io.AddFunc(0x8b72cda1, cellPadGetData);
	sys_io.AddFunc(0x6bc09c61, cellPadGetDataExtra);
	sys_io.AddFunc(0xf65544ee, cellPadSetActDirect);
	sys_io.AddFunc(0xa703a51d, cellPadGetInfo2);
	sys_io.AddFunc(0x578e3c98, cellPadSetPortSetting);
}
