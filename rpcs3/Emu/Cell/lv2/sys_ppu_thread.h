#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"

class ppu_thread;

enum : s32
{
	SYS_PPU_THREAD_ONCE_INIT = 0,
	SYS_PPU_THREAD_DONE_INIT = 1,
};

// PPU Thread Flags
enum : u64
{
	SYS_PPU_THREAD_CREATE_JOINABLE = 0x1,
	SYS_PPU_THREAD_CREATE_INTERRUPT = 0x2,
};

struct sys_ppu_thread_stack_t
{
	be_t<u32> pst_addr;
	be_t<u32> pst_size;
};

struct ppu_thread_param_t
{
	vm::bptr<void(u64)> entry;
	be_t<u32> tls; // vm::bptr<void>
};

struct sys_ppu_thread_icontext_t
{
	be_t<u64> gpr[32];
	be_t<u32> cr;
	be_t<u32> rsv1;
	be_t<u64> xer;
	be_t<u64> lr;
	be_t<u64> ctr;
	be_t<u64> pc;
};

// Syscalls

void _sys_ppu_thread_exit(ppu_thread& ppu, u64 errorcode);
s32 sys_ppu_thread_yield(ppu_thread& ppu); // Return value is ignored by the library
error_code sys_ppu_thread_join(ppu_thread& ppu, u32 thread_id, vm::ptr<u64> vptr);
error_code sys_ppu_thread_detach(ppu_thread& ppu, u32 thread_id);
error_code sys_ppu_thread_get_join_state(ppu_thread& ppu, vm::ptr<s32> isjoinable); // Error code is ignored by the library
error_code sys_ppu_thread_set_priority(ppu_thread& ppu, u32 thread_id, s32 prio);
error_code sys_ppu_thread_get_priority(ppu_thread& ppu, u32 thread_id, vm::ptr<s32> priop);
error_code sys_ppu_thread_get_stack_information(ppu_thread& ppu, vm::ptr<sys_ppu_thread_stack_t> sp);
error_code sys_ppu_thread_stop(ppu_thread& ppu, u32 thread_id);
error_code sys_ppu_thread_restart(ppu_thread& ppu);
error_code _sys_ppu_thread_create(ppu_thread& ppu, vm::ptr<u64> thread_id, vm::ptr<ppu_thread_param_t> param, u64 arg, u64 arg4, s32 prio, u64 stacksize, u64 flags, vm::cptr<char> threadname);
error_code sys_ppu_thread_start(ppu_thread& ppu, u32 thread_id);
error_code sys_ppu_thread_rename(ppu_thread& ppu, u32 thread_id, vm::cptr<char> name);
error_code sys_ppu_thread_recover_page_fault(ppu_thread& ppu, u32 thread_id);
error_code sys_ppu_thread_get_page_fault_context(ppu_thread& ppu, u32 thread_id, vm::ptr<sys_ppu_thread_icontext_t> ctxt);
