#pragma once

#include "Emu/Memory/vm_ptr.h"

// SysCalls

error_code sys_sm_get_ext_event2(vm::ptr<u64> a1, vm::ptr<u64> a2, vm::ptr<u64> a3, u64 a4);
error_code sys_sm_shutdown(u16 op, vm::ptr<void> param, u64 size);
error_code sys_sm_get_params(vm::ptr<u8> a, vm::ptr<u8> b, vm::ptr<u32> c, vm::ptr<u64> d);
error_code sys_sm_control_led(u8 led, u8 action);
error_code sys_sm_ring_buzzer(u64 packet, u64 a1, u64 a2);
