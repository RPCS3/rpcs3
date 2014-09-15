#pragma once

#define MHZ (1000000)

// Auxiliary functions
u64 get_time();
u64 get_system_time();

// SysCalls
s32 sys_time_get_timezone(vm::ptr<be_t<u32>> timezone, vm::ptr<be_t<u32>> summertime);
s32 sys_time_get_current_time(u32 sec_addr, u32 nsec_addr);
s64 sys_time_get_system_time();
u64 sys_time_get_timebase_frequency();
