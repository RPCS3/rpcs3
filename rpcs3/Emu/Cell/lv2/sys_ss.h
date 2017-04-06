#pragma once

#include "Emu/Memory/Memory.h"
#include "Emu/Cell/ErrorCodes.h"

struct CellSsOpenPSID
{
	be_t<u64> high;
	be_t<u64> low;
};

s32 sys_ss_get_console_id(vm::ps3::ptr<u8> buf);
s32 sys_ss_get_open_psid(vm::ps3::ptr<CellSsOpenPSID> ptr);
