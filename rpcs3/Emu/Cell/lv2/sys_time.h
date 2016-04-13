#pragma once

namespace vm { using namespace ps3; }

// SysCalls
s32 sys_time_get_timezone(vm::ptr<s32> timezone, vm::ptr<s32> summertime);
s32 sys_time_get_current_time(vm::ptr<s64> sec, vm::ptr<s64> nsec);
u64 sys_time_get_timebase_frequency();
