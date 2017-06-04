#pragma once

namespace vm { using namespace ps3; }

class PPUThread;

struct lv2_int_tag_t
{
	const u32 id;

	std::shared_ptr<struct lv2_int_serv_t> handler;

	lv2_int_tag_t();
};

struct lv2_int_serv_t
{
	const std::shared_ptr<PPUThread> thread;
	const u32 id;

	std::atomic<u32> signal{ 0 }; // signal count

	lv2_int_serv_t(const std::shared_ptr<PPUThread>& thread);

	void join(PPUThread& ppu, lv2_lock_t& lv2_lock);
};

// SysCalls
s32 sys_interrupt_tag_destroy(u32 intrtag);
s32 _sys_interrupt_thread_establish(vm::ptr<u32> ih, u32 intrtag, u32 intrthread, u64 arg1, u64 arg2);
s32 _sys_interrupt_thread_disestablish(PPUThread& ppu, u32 ih, vm::ptr<u64> r13);
void sys_interrupt_thread_eoi(PPUThread& ppu);
