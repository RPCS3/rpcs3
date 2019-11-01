#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"

struct CellSsOpenPSID
{
	be_t<u64> high;
	be_t<u64> low;
};

error_code sys_ss_random_number_generator(u32 arg1, vm::ptr<void> buf, u64 size);
error_code sys_ss_access_control_engine(u64 pkg_id, u64 a2, u64 a3);
s32 sys_ss_get_console_id(vm::ptr<u8> buf);
s32 sys_ss_get_open_psid(vm::ptr<CellSsOpenPSID> ptr);
