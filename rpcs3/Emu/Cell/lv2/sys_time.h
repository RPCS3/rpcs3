#pragma once

#include "Emu/Memory/vm_ptr.h"

// SysCalls

error_code sys_time_get_timezone(vm::ptr<s32> timezone, vm::ptr<s32> summertime);
error_code sys_time_get_current_time(vm::ptr<s64> sec, vm::ptr<s64> nsec);
u64 sys_time_get_timebase_frequency();
error_code sys_time_get_rtc(vm::ptr<u64> rtc);
