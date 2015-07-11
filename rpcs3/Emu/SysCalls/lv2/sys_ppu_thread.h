#pragma once

namespace vm { using namespace ps3; }

class PPUThread;

enum : u32
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
	be_t<u32> entry; // vm::bptr<void(u64)>
	be_t<u32> tls; // vm::bptr<void>
};

enum : u32
{
	PPU_THREAD_STATUS_IDLE,
	PPU_THREAD_STATUS_RUNNABLE,
	PPU_THREAD_STATUS_ONPROC,
	PPU_THREAD_STATUS_SLEEP,
	PPU_THREAD_STATUS_STOP,
	PPU_THREAD_STATUS_ZOMBIE,
	PPU_THREAD_STATUS_DELETED,
	PPU_THREAD_STATUS_UNKNOWN,
};

// Aux
u32 ppu_thread_create(u32 entry, u64 arg, s32 prio, u32 stacksize, bool is_joinable, bool is_interrupt, std::string name, std::function<void(PPUThread&)> task = nullptr);

// SysCalls
void _sys_ppu_thread_exit(PPUThread& ppu, u64 errorcode);
void sys_ppu_thread_yield();
s32 sys_ppu_thread_join(PPUThread& ppu, u32 thread_id, vm::ptr<u64> vptr);
s32 sys_ppu_thread_detach(u32 thread_id);
void sys_ppu_thread_get_join_state(PPUThread& ppu, vm::ptr<s32> isjoinable);
s32 sys_ppu_thread_set_priority(u32 thread_id, s32 prio);
s32 sys_ppu_thread_get_priority(u32 thread_id, vm::ptr<s32> priop);
s32 sys_ppu_thread_get_stack_information(PPUThread& ppu, vm::ptr<sys_ppu_thread_stack_t> sp);
s32 sys_ppu_thread_stop(u32 thread_id);
s32 sys_ppu_thread_restart(u32 thread_id);
s32 _sys_ppu_thread_create(vm::ptr<u64> thread_id, vm::ptr<ppu_thread_param_t> param, u64 arg, u64 arg4, s32 prio, u32 stacksize, u64 flags, vm::cptr<char> threadname);
s32 sys_ppu_thread_start(u32 thread_id);
s32 sys_ppu_thread_rename(u32 thread_id, vm::cptr<char> name);
