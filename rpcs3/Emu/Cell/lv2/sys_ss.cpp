#include "stdafx.h"
#include "Emu/Cell/PPUThread.h"

#include "sys_ss.h"

namespace vm { using namespace ps3; }

logs::channel sys_ss("sys_ss", logs::level::notice);

s32 sys_ss_get_console_id(vm::ps3::ptr<u8> buf)
{
	sys_ss.todo("sys_ss_get_console_id(buf=*0x%x)", buf);

	// TODO: Return some actual IDPS?
	*buf = 0;

	return CELL_OK;
}

s32 sys_ss_get_open_psid(vm::ps3::ptr<CellSsOpenPSID> psid) 
{
	sys_ss.warning("sys_ss_get_open_psid(psid=*0x%x)", psid);

	psid->high = 0;
	psid->low = 0;

	return CELL_OK;
}
