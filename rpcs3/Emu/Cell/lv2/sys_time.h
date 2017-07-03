#pragma once

// SysCalls
s32 sys_time_get_timezone(vm::ps3::ptr<s32> timezone, vm::ps3::ptr<s32> summertime);
s32 sys_time_get_current_time(vm::ps3::ptr<s64> sec, vm::ps3::ptr<s64> nsec);
u64 sys_time_get_timebase_frequency();
