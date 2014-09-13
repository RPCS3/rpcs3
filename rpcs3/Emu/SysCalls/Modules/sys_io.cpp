#include "stdafx.h"
#include "Emu/SysCalls/Modules.h"

Module *sys_io = nullptr;

extern void cellPad_init();
extern void cellKb_init();
extern void cellMouse_init();

void sys_io_init(Module *pxThis)
{
	sys_io = pxThis;

	cellPad_init();
	cellKb_init();
	cellMouse_init();
}
