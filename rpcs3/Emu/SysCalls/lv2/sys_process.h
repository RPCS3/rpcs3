#pragma once

//Process Local Object
enum
{
	SYS_MEM_OBJECT                   = (0x08UL),
	SYS_MUTEX_OBJECT                 = (0x85UL),
	SYS_COND_OBJECT                  = (0x86UL),
	SYS_RWLOCK_OBJECT                = (0x88UL),
	SYS_INTR_TAG_OBJECT              = (0x0AUL),
	SYS_INTR_SERVICE_HANDLE_OBJECT   = (0x0BUL),
	SYS_EVENT_QUEUE_OBJECT           = (0x8DUL),
	SYS_EVENT_PORT_OBJECT            = (0x0EUL),
	SYS_TRACE_OBJECT                 = (0x21UL),
	SYS_SPUIMAGE_OBJECT              = (0x22UL),
	SYS_PRX_OBJECT                   = (0x23UL),
	SYS_SPUPORT_OBJECT               = (0x24UL),
	SYS_LWMUTEX_OBJECT               = (0x95UL),
	SYS_TIMER_OBJECT                 = (0x11UL),
	SYS_SEMAPHORE_OBJECT             = (0x96UL),
	SYS_FS_FD_OBJECT                 = (0x73UL),
	SYS_LWCOND_OBJECT                = (0x97UL),
	SYS_EVENT_FLAG_OBJECT            = (0x98UL),
};

// Auxiliary functions
s32 process_getpid();
s32 process_get_sdk_version(u32 pid, s32& ver);
s32 process_is_spu_lock_line_reservation_address(u32 addr, u64 flags);

// SysCalls
s32 sys_process_getpid();
s32 sys_process_getppid();
s32 sys_process_get_number_of_object(u32 object, vm::ptr<be_t<u32>> nump);
s32 sys_process_get_id(u32 object, vm::ptr<be_t<u32>> buffer, u32 size, vm::ptr<be_t<u32>> set_size);
s32 sys_process_get_paramsfo(vm::ptr<u8> buffer);
s32 sys_process_get_sdk_version(u32 pid, vm::ptr<be_t<s32>> version);
s32 sys_process_get_status(u64 unk);
s32 sys_process_is_spu_lock_line_reservation_address(u32 addr, u64 flags);
s32 sys_process_exit(s32 errorcode);
s32 sys_process_kill(u32 pid);
s32 sys_process_wait_for_child(u32 pid, vm::ptr<be_t<u32>> status, u64 unk);
s32 sys_process_wait_for_child2(u64 unk1, u64 unk2, u64 unk3, u64 unk4, u64 unk5, u64 unk6);
s32 sys_process_detach_child(u64 unk);
void sys_game_process_exitspawn(vm::ptr<const char> path, u32 argv_addr, u32 envp_addr,
								u32 data_addr, u32 data_size, u32 prio, u64 flags);
void sys_game_process_exitspawn2(vm::ptr<const char> path, u32 argv_addr, u32 envp_addr,
								u32 data_addr, u32 data_size, u32 prio, u64 flags);
