#include "stdafx.h"
#include "Emu/SysCalls/Modules.h"

extern Module sys_io;

extern void cellPad_init();
extern void cellKb_init();
extern void cellMouse_init();

Module sys_io("sys_io", []()
{
	cellPad_init();
	cellKb_init();
	cellMouse_init();
});
