#pragma once

#include "sys_sync.h"

class ppu_thread;

struct lv2_int_tag_t
{
	static const u32 id_base = 0x1000a;
	static const u32 id_step = 0x10000;
	static const u32 id_count = 8192;

	const u32 id;

	std::shared_ptr<struct lv2_int_serv_t> handler;

	lv2_int_tag_t();
};

struct lv2_int_serv_t
{
	static const u32 id_base = 0x0b000000;
	static const u32 id_step = 0x100;
	static const u32 id_count = 8192;

	const std::shared_ptr<ppu_thread> thread;

	const u32 id;

	const u64 arg1;
	const u64 arg2;

	lv2_int_serv_t(const std::shared_ptr<ppu_thread>& thread, u64 arg1, u64 arg2);

	void exec();
	void join(ppu_thread& ppu, lv2_lock_t);
};

// SysCalls
s32 sys_interrupt_tag_destroy(u32 intrtag);
s32 _sys_interrupt_thread_establish(vm::ps3::ptr<u32> ih, u32 intrtag, u32 intrthread, u64 arg1, u64 arg2);
s32 _sys_interrupt_thread_disestablish(ppu_thread& ppu, u32 ih, vm::ps3::ptr<u64> r13);
void sys_interrupt_thread_eoi(ppu_thread& ppu);
