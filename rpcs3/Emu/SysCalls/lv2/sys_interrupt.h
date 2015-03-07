#pragma once

class PPUThread;

struct interrupt_handler_t
{
	std::shared_ptr<CPUThread> handler;
};

// SysCalls
s32 sys_interrupt_tag_destroy(u32 intrtag);
s32 sys_interrupt_thread_establish(vm::ptr<u32> ih, u32 intrtag, u64 intrthread, u64 arg);
s32 _sys_interrupt_thread_disestablish(u32 ih, vm::ptr<u64> r13);
void sys_interrupt_thread_eoi();
