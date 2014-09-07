#include "stdafx.h"
#include "Emu/SysCalls/Modules.h"

//void sys_io_init();
//Module sys_io(0x0017, sys_io_init);
Module *sys_io = nullptr;

extern void cellPad_init();
extern void cellKb_init();
extern void cellMouse_init();

void sys_io_init()
{
	cellPad_init();
	cellKb_init();
	cellMouse_init();
}
