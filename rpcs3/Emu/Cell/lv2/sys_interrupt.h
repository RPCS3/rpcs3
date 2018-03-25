#pragma once

#include "sys_sync.h"

class ppu_thread;

struct lv2_int_tag final : lv2_obj
{
	static const u32 id_base = 0x0a000000;

	std::weak_ptr<struct lv2_int_serv> handler;
};

struct lv2_int_serv final : lv2_obj
{
	static const u32 id_base = 0x0b000000;

	const std::shared_ptr<ppu_thread> thread;
	const u64 arg1;
	const u64 arg2;

	lv2_int_serv(const std::shared_ptr<ppu_thread>& thread, u64 arg1, u64 arg2)
		: thread(thread)
		, arg1(arg1)
		, arg2(arg2)
	{
	}

	void exec();
	void join();
};

// Syscalls

error_code sys_interrupt_tag_destroy(u32 intrtag);
error_code _sys_interrupt_thread_establish(vm::ptr<u32> ih, u32 intrtag, u32 intrthread, u64 arg1, u64 arg2);
error_code _sys_interrupt_thread_disestablish(ppu_thread& ppu, u32 ih, vm::ptr<u64> r13);
void sys_interrupt_thread_eoi(ppu_thread& ppu);
