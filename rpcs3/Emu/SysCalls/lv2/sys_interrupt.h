#pragma once

// SysCalls
s32 sys_interrupt_tag_destroy(u32 intrtag);
s32 sys_interrupt_thread_establish(vm::ptr<be_t<u32>> ih, u32 intrtag, u64 intrthread, u64 arg);
s32 sys_interrupt_thread_disestablish(u32 ih);
void sys_interrupt_thread_eoi();
