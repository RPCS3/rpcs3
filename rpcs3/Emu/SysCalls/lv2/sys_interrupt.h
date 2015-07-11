#pragma once

namespace vm { using namespace ps3; }

class PPUThread;

struct lv2_int_handler_t
{
	const std::shared_ptr<PPUThread> thread;

	lv2_int_handler_t(const std::shared_ptr<PPUThread>& thread)
		: thread(thread)
	{
	}
};

REG_ID_TYPE(lv2_int_handler_t, 0x0B); // SYS_INTR_SERVICE_HANDLE_OBJECT

// SysCalls
s32 sys_interrupt_tag_destroy(u32 intrtag);
s32 sys_interrupt_thread_establish(vm::ptr<u32> ih, u32 intrtag, u32 intrthread, u64 arg);
s32 _sys_interrupt_thread_disestablish(u32 ih, vm::ptr<u64> r13);
void sys_interrupt_thread_eoi(PPUThread& CPU);
