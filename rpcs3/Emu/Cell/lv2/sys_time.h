#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"

// SysCalls

class ppu_thread;

error_code sys_time_set_timezone(ppu_thread& ppu, s32 timezone, s32 summertime);
error_code sys_time_get_timezone(vm::ptr<s32> timezone, vm::ptr<s32> summertime);
error_code sys_time_get_current_time(vm::ptr<s64> sec, vm::ptr<s64> nsec);
error_code sys_time_set_current_time(ppu_thread& ppu, s64 sec, s64 nsec);
u64 sys_time_get_timebase_frequency();
error_code sys_time_get_rtc(vm::ptr<u64> rtc);

extern u64 g_timebase_offs;
