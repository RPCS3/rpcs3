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

// Datatypes
// TODO: Would it be better if improved the existing IDManager, instead of creating this datatype?
//       Another option would be improving SysCallBase::GetNewId
struct sysProcessObjects_t
{
	std::set<u32> mem_objects;
	std::set<u32> mutex_objects;
	std::set<u32> cond_objects;
	std::set<u32> rwlock_objects;
	std::set<u32> intr_tag_objects;
	std::set<u32> intr_service_handle_objects;
	std::set<u32> event_queue_objects;
	std::set<u32> event_port_objects;
	std::set<u32> trace_objects;
	std::set<u32> spuimage_objects;
	std::set<u32> prx_objects;
	std::set<u32> spuport_objects;
	std::set<u32> lwmutex_objects;
	std::set<u32> timer_objects;
	std::set<u32> semaphore_objects;
	std::set<u32> fs_fd_objects;
	std::set<u32> lwcond_objects;
	std::set<u32> event_flag_objects;
};

// Extern
extern sysProcessObjects_t procObjects;

// SysCalls
int sys_process_getpid();
int sys_process_getppid();
int sys_process_get_number_of_object(u32 object, mem32_t nump);
int sys_process_get_id(u32 object, mem32_ptr_t buffer, u32 size, mem32_t set_size);
int sys_process_get_paramsfo(mem8_ptr_t buffer);
int sys_process_get_sdk_version(u32 pid, mem32_t version);
int sys_process_get_status(u64 unk);
int sys_process_exit(s32 errorcode);
int sys_process_kill(u32 pid);
int sys_process_wait_for_child(u32 pid, mem32_t status, u64 unk);
int sys_process_wait_for_child2(u64 unk1, u64 unk2, u64 unk3, u64 unk4, u64 unk5, u64 unk6);
int sys_process_detach_child(u64 unk);
void sys_game_process_exitspawn(u32 path_addr, u32 argv_addr, u32 envp_addr,
								u32 data_addr, u32 data_size, u32 prio, u64 flags);
void sys_game_process_exitspawn2(u32 path_addr, u32 argv_addr, u32 envp_addr,
								u32 data_addr, u32 data_size, u32 prio, u64 flags);
