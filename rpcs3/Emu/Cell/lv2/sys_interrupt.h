#pragma once

#include "sys_sync.h"

#include "Emu/Memory/vm_ptr.h"

class ppu_thread;

struct lv2_int_tag final : public lv2_obj
{
	static const u32 id_base = 0x0a000000;

	const u32 id;
	shared_ptr<struct lv2_int_serv> handler;

	lv2_int_tag() noexcept;
	lv2_int_tag(utils::serial& ar) noexcept;
	void save(utils::serial& ar);
};

struct lv2_int_serv final : public lv2_obj
{
	static const u32 id_base = 0x0b000000;

	const u32 id;
	const shared_ptr<named_thread<ppu_thread>> thread;
	const u64 arg1;
	const u64 arg2;

	lv2_int_serv(shared_ptr<named_thread<ppu_thread>> thread, u64 arg1, u64 arg2) noexcept;
	lv2_int_serv(utils::serial& ar) noexcept;
	void save(utils::serial& ar);

	void exec() const;
	void join() const;
};

// Syscalls

error_code sys_interrupt_tag_destroy(ppu_thread& ppu, u32 intrtag);
error_code _sys_interrupt_thread_establish(ppu_thread& ppu, vm::ptr<u32> ih, u32 intrtag, u32 intrthread, u64 arg1, u64 arg2);
error_code _sys_interrupt_thread_disestablish(ppu_thread& ppu, u32 ih, vm::ptr<u64> r13);
void sys_interrupt_thread_eoi(ppu_thread& ppu);
