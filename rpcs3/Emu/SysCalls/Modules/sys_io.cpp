#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void sys_io_init();
Module sys_io(0x0017, sys_io_init);

void sys_io_init()
{
	sys_io.AddFunc(0x1cf98800, bind_func(cellPadInit));
	sys_io.AddFunc(0x4d9b75d5, bind_func(cellPadEnd));
	sys_io.AddFunc(0x0d5f2c14, bind_func(cellPadClearBuf));
	sys_io.AddFunc(0x8b72cda1, bind_func(cellPadGetData));
	sys_io.AddFunc(0x6bc09c61, bind_func(cellPadGetDataExtra));
	sys_io.AddFunc(0xf65544ee, bind_func(cellPadSetActDirect));
	sys_io.AddFunc(0xa703a51d, bind_func(cellPadGetInfo2));
	sys_io.AddFunc(0x578e3c98, bind_func(cellPadSetPortSetting));
}
