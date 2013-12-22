#pragma once
#include "ErrorCodes.h"
#include "lv2/SC_FileSystem.h"
#include "lv2/SC_Timer.h"
#include "lv2/SC_Rwlock.h"
#include "lv2/SC_SPU_Thread.h"
//#define SYSCALLS_DEBUG

#define declCPU PPUThread& CPU = GetCurrentPPUThread

extern bool enable_log;

class SysCallBase //Module
{
private:
	wxString m_module_name;
	//u32 m_id;

public:
	SysCallBase(const wxString& name/*, u32 id*/)
		: m_module_name(name)
		//, m_id(id)
	{
	}

	const wxString& GetName() const { return m_module_name; }

	void Log(const u32 id, wxString fmt, ...)
	{
		if(enable_log)
		{
		va_list list;
		va_start(list, fmt);
		ConLog.Write(GetName() + wxString::Format("[%d]: ", id) + wxString::FormatV(fmt, list));
		va_end(list);
		}
	}

	void Log(wxString fmt, ...)
	{
		if(enable_log)
		{
		va_list list;
		va_start(list, fmt);
		ConLog.Write(GetName() + ": " + wxString::FormatV(fmt, list));
		va_end(list);
		}
	}

	void Warning(const u32 id, wxString fmt, ...)
	{
//#ifdef SYSCALLS_DEBUG
		va_list list;
		va_start(list, fmt);
		ConLog.Warning(GetName() + wxString::Format("[%d] warning: ", id) + wxString::FormatV(fmt, list));
		va_end(list);
//#endif
	}

	void Warning(wxString fmt, ...)
	{
//#ifdef SYSCALLS_DEBUG
		va_list list;
		va_start(list, fmt);
		ConLog.Warning(GetName() + " warning: " + wxString::FormatV(fmt, list));
		va_end(list);
//#endif
	}

	void Error(const u32 id, wxString fmt, ...)
	{
		va_list list;
		va_start(list, fmt);
		ConLog.Error(GetName() + wxString::Format("[%d] error: ", id) + wxString::FormatV(fmt, list));
		va_end(list);
	}

	void Error(wxString fmt, ...)
	{
		va_list list;
		va_start(list, fmt);
		ConLog.Error(GetName() + " error: " + wxString::FormatV(fmt, list));
		va_end(list);
	}

	bool CheckId(u32 id) const
	{
		return Emu.GetIdManager().CheckID(id) && !Emu.GetIdManager().GetIDData(id).m_name.Cmp(GetName());
	}

	bool CheckId(u32 id, ID& _id) const
	{
		return Emu.GetIdManager().CheckID(id) && !(_id = Emu.GetIdManager().GetIDData(id)).m_name.Cmp(GetName());
	}

	template<typename T> bool CheckId(u32 id, T*& data)
	{
		ID id_data;

		if(!CheckId(id, id_data)) return false;

		data = (T*)id_data.m_data;

		return true;
	}

	u32 GetNewId(void* data = nullptr, u8 flags = 0)
	{
		return Emu.GetIdManager().GetNewID(GetName(), data, flags);
	}
};

//process
extern int sys_process_getpid();
extern int sys_process_getppid();
extern int sys_process_get_number_of_object(u32 object, mem32_t nump);
extern int sys_process_get_id(u32 object, mem8_ptr_t buffer, u32 size, mem32_t set_size);
extern int sys_process_get_paramsfo(mem8_ptr_t buffer);
extern int sys_process_exit(int errorcode);
extern int sys_process_is_stack(u32 addr); //TODO: Is this a lv2 SysCall? If so, where is its number?
extern int sys_game_process_exitspawn(u32 path_addr, u32 argv_addr, u32 envp_addr,
								u32 data, u32 data_size, int prio, u64 flags );

//sys_event
extern int sys_event_flag_create(u32 eflag_id_addr, u32 attr_addr, u64 init);
extern int sys_event_flag_destroy(u32 eflag_id);
extern int sys_event_flag_wait(u32 eflag_id, u64 bitptn, u32 mode, u32 result_addr, u32 timeout);
extern int sys_event_flag_trywait(u32 eflag_id, u64 bitptn, u32 mode, u32 result_addr);
extern int sys_event_flag_set(u32 eflag_id, u64 bitptn);
extern int sys_event_flag_clear(u32 eflag_id, u64 bitptn);
extern int sys_event_flag_cancel(u32 eflag_id, u32 num_addr);
extern int sys_event_flag_get(u32 eflag_id, u32 flag_addr);
extern int sys_event_queue_create(u32 equeue_id_addr, u32 attr_addr, u64 event_queue_key, int size);
extern int sys_event_queue_receive(u32 equeue_id, u32 event_addr, u32 timeout);
extern int sys_event_port_create(u32 eport_id_addr, int port_type, u64 name);
extern int sys_event_port_connect_local(u32 event_port_id, u32 event_queue_id);
extern int sys_event_port_send(u32 event_port_id, u64 data1, u64 data2, u64 data3);

//sys_semaphore
extern int sys_semaphore_create(u32 sem_addr, u32 attr_addr, int initial_val, int max_val);
extern int sys_semaphore_destroy(u32 sem);
extern int sys_semaphore_wait(u32 sem, u64 timeout);
extern int sys_semaphore_trywait(u32 sem);
extern int sys_semaphore_post(u32 sem, int count);
extern int sys_semaphore_get_value(u32 sem, u32 count_addr);

//sys_lwmutex
extern int sys_lwmutex_create(u64 lwmutex_addr, u64 lwmutex_attr_addr);
extern int sys_lwmutex_destroy(u64 lwmutex_addr);
extern int sys_lwmutex_lock(u64 lwmutex_addr, u64 timeout);
extern int sys_lwmutex_trylock(u64 lwmutex_addr);
extern int sys_lwmutex_unlock(u64 lwmutex_addr);

//sys_cond
extern int sys_cond_create(u32 cond_addr, u32 mutex_id, u32 attr_addr);
extern int sys_cond_destroy(u32 cond_id);
extern int sys_cond_wait(u32 cond_id, u64 timeout);
extern int sys_cond_signal(u32 cond_id);
extern int sys_cond_signal_all(u32 cond_id);

//sys_mutex
extern int sys_mutex_create(u32 mutex_id_addr, u32 attr_addr);
extern int sys_mutex_destroy(u32 mutex_id);
extern int sys_mutex_lock(u32 mutex_id, u64 timeout);
extern int sys_mutex_trylock(u32 mutex_id);
extern int sys_mutex_unlock(u32 mutex_id);

//ppu_thread
extern int sys_ppu_thread_exit(int errorcode);
extern int sys_ppu_thread_yield();
extern int sys_ppu_thread_join(u32 thread_id, u32 vptr_addr);
extern int sys_ppu_thread_detach(u32 thread_id);
extern void sys_ppu_thread_get_join_state(u32 isjoinable_addr);
extern int sys_ppu_thread_set_priority(u32 thread_id, int prio);
extern int sys_ppu_thread_get_priority(u32 thread_id, u32 prio_addr);
extern int sys_ppu_thread_get_stack_information(u32 info_addr);
extern int sys_ppu_thread_stop(u32 thread_id);
extern int sys_ppu_thread_restart(u32 thread_id);
extern int sys_ppu_thread_create(u32 thread_id_addr, u32 entry, u32 arg, int prio, u32 stacksize, u64 flags, u32 threadname_addr);
extern void sys_ppu_thread_once(u32 once_ctrl_addr, u32 entry);
extern int sys_ppu_thread_get_id(const u32 id_addr);
extern int sys_spu_thread_group_connect_event_all_threads(u32 id, u32 eq, u64 req, u32 spup_addr);

//memory
extern int sys_memory_container_create(u32 cid_addr, u32 yield_size);
extern int sys_memory_container_destroy(u32 cid);
extern int sys_memory_allocate(u32 size, u32 flags, u32 alloc_addr_addr);
extern int sys_memory_free(u32 start_addr);
extern int sys_memory_get_user_memory_size(u32 mem_info_addr);
extern int sys_mmapper_allocate_address(u32 size, u64 flags, u32 alignment, u32 alloc_addr);
extern int sys_mmapper_allocate_memory(u32 size, u64 flags, u32 mem_id_addr);
extern int sys_mmapper_map_memory(u32 start_addr, u32 mem_id, u64 flags);

//vm
extern int sys_vm_memory_map(u32 vsize, u32 psize, u32 cid, u64 flag, u64 policy, u32 addr);
extern int sys_vm_unmap(u32 addr);
extern int sys_vm_append_memory(u32 addr, u32 size);
extern int sys_vm_return_memory(u32 addr, u32 size);
extern int sys_vm_lock(u32 addr, u32 size);
extern int sys_vm_unlock(u32 addr, u32 size);
extern int sys_vm_touch(u32 addr, u32 size);
extern int sys_vm_flush(u32 addr, u32 size);
extern int sys_vm_invalidate(u32 addr, u32 size);
extern int sys_vm_store(u32 addr, u32 size);
extern int sys_vm_sync(u32 addr, u32 size);
extern int sys_vm_test(u32 addr, u32 size, u32 result_addr);
extern int sys_vm_get_statistics(u32 addr, u32 stat_addr);

//cellFs
extern int cellFsOpen(u32 path_addr, int flags, mem32_t fd, mem32_t arg, u64 size);
extern int cellFsRead(u32 fd, u32 buf_addr, u64 nbytes, mem64_t nread);
extern int cellFsWrite(u32 fd, u32 buf_addr, u64 nbytes, mem64_t nwrite);
extern int cellFsClose(u32 fd);
extern int cellFsOpendir(u32 path_addr, mem32_t fd);
extern int cellFsReaddir(u32 fd, u32 dir_addr, mem64_t nread);
extern int cellFsClosedir(u32 fd);
extern int cellFsStat(u32 path_addr, mem_ptr_t<CellFsStat> sb);
extern int cellFsFstat(u32 fd, mem_ptr_t<CellFsStat> sb);
extern int cellFsMkdir(u32 path_addr, u32 mode);
extern int cellFsRename(u32 from_addr, u32 to_addr);
extern int cellFsRmdir(u32 path_addr);
extern int cellFsUnlink(u32 path_addr);
extern int cellFsLseek(u32 fd, s64 offset, u32 whence, mem64_t pos);
extern int cellFsFtruncate(u32 fd, u64 size);
extern int cellFsTruncate(u32 path_addr, u64 size);
extern int cellFsFGetBlockSize(u32 fd, mem64_t sector_size, mem64_t block_size);

//cellVideo
extern int cellVideoOutGetState(u32 videoOut, u32 deviceIndex, u32 state_addr);
extern int cellVideoOutGetResolution(u32 resolutionId, u32 resolution_addr);
extern int cellVideoOutConfigure(u32 videoOut, u32 config_addr, u32 option_addr, u32 waitForEvent);
extern int cellVideoOutGetConfiguration(u32 videoOut, u32 config_addr, u32 option_addr);
extern int cellVideoOutGetNumberOfDevice(u32 videoOut);
extern int cellVideoOutGetResolutionAvailability(u32 videoOut, u32 resolutionId, u32 aspect, u32 option);

//cellPad
extern int cellPadInit(u32 max_connect);
extern int cellPadEnd();
extern int cellPadClearBuf(u32 port_no);
extern int cellPadGetData(u32 port_no, u32 data_addr);
extern int cellPadGetDataExtra(u32 port_no, u32 device_type_addr, u32 data_addr);
extern int cellPadSetActDirect(u32 port_no, u32 param_addr);
extern int cellPadGetInfo(u32 info_addr);
extern int cellPadGetInfo2(u32 info_addr);
extern int cellPadSetPortSetting(u32 port_no, u32 port_setting);

//cellKb
extern int cellKbInit(u32 max_connect);
extern int cellKbEnd();
extern int cellKbClearBuf(u32 port_no);
extern u16 cellKbCnvRawCode(u32 arrange, u32 mkey, u32 led, u16 rawcode);
extern int cellKbGetInfo(mem_class_t info);
extern int cellKbRead(u32 port_no, mem_class_t data);
extern int cellKbSetCodeType(u32 port_no, u32 type);
extern int cellKbSetLEDStatus(u32 port_no, u8 led);
extern int cellKbSetReadMode(u32 port_no, u32 rmode);
extern int cellKbGetConfiguration(u32 port_no, mem_class_t config);

//cellMouse
extern int cellMouseInit(u32 max_connect);
extern int cellMouseClearBuf(u32 port_no);
extern int cellMouseEnd();
extern int cellMouseGetInfo(mem_class_t info);
extern int cellMouseInfoTabletMode(u32 port_no, mem_class_t info);
extern int cellMouseGetData(u32 port_no, mem_class_t data);
extern int cellMouseGetDataList(u32 port_no, mem_class_t data);
extern int cellMouseSetTabletMode(u32 port_no, u32 mode);
extern int cellMouseGetTabletDataList(u32 port_no, mem_class_t data);
extern int cellMouseGetRawData(u32 port_no, mem_class_t data);

//cellGcm
extern int cellGcmCallback(u32 context_addr, u32 count);

//sys_tty
extern int sys_tty_read(u32 ch, u64 buf_addr, u32 len, u64 preadlen_addr);
extern int sys_tty_write(u32 ch, u64 buf_addr, u32 len, u64 pwritelen_addr);

//sys_heap
extern int sys_heap_create_heap(const u32 heap_addr, const u32 start_addr, const u32 size);
extern int sys_heap_malloc(const u32 heap_addr, const u32 size);

//sys_spu
extern int sys_spu_image_open(mem_ptr_t<sys_spu_image> img, u32 path_addr);
extern int sys_spu_thread_initialize(mem32_t thread, u32 group, u32 spu_num, mem_ptr_t<sys_spu_image> img, mem_ptr_t<sys_spu_thread_attribute> attr, mem_ptr_t<sys_spu_thread_argument> arg);
extern int sys_spu_thread_set_argument(u32 id, mem_ptr_t<sys_spu_thread_argument> arg);
extern int sys_spu_thread_group_start(u32 id);
extern int sys_spu_thread_group_create(mem32_t id, u32 num, int prio, mem_ptr_t<sys_spu_thread_group_attribute> attr);
extern int sys_spu_thread_create(mem32_t thread_id, mem32_t entry, u64 arg, int prio, u32 stacksize, u64 flags, u32 threadname_addr);
extern int sys_spu_thread_connect_event(u32 id, u32 eq, u32 et, u8 spup);
extern int sys_raw_spu_create(mem32_t id, u32 attr_addr);
extern int sys_spu_initialize(u32 max_usable_spu, u32 max_raw_spu);
extern int sys_spu_thread_write_ls(u32 id, u32 address, u64 value, u32 type);
extern int sys_spu_thread_read_ls(u32 id, u32 address, mem64_t value, u32 type);
extern int sys_spu_thread_write_spu_mb(u32 id, u32 value);
extern int sys_spu_thread_set_spu_cfg(u32 id, u64 value);
extern int sys_spu_thread_get_spu_cfg(u32 id, mem64_t value);
extern int sys_spu_thread_write_snr(u32 id, u32 number, u32 value);

//sys_time
extern int sys_time_get_timezone(mem32_t timezone, mem32_t summertime);
extern int sys_time_get_current_time(u32 sec_addr, u32 nsec_addr);
extern s64 sys_time_get_system_time();
extern u64 sys_time_get_timebase_frequency();

//sys_timer
extern int sys_timer_create(mem32_t timer_id);
extern int sys_timer_destroy(u32 timer_id);
extern int sys_timer_get_information(u32 timer_id, mem_ptr_t<sys_timer_information_t> info);
extern int sys_timer_start(u32 timer_id, s64 basetime, u64 period);
extern int sys_timer_stop(u32 timer_id);
extern int sys_timer_connect_event_queue(u32 timer_id, u32 queue_id, u64 name, u64 data1, u64 data2);
extern int sys_timer_disconnect_event_queue(u32 timer_id);
extern int sys_timer_sleep(u32 sleep_time);
extern int sys_timer_usleep(u64 sleep_time);

//sys_trace
extern int sys_trace_create();
extern int sys_trace_start();
extern int sys_trace_stop();
extern int sys_trace_update_top_index();
extern int sys_trace_destroy();
extern int sys_trace_drain();
extern int sys_trace_attach_process();
extern int sys_trace_allocate_buffer();
extern int sys_trace_free_buffer();
extern int sys_trace_create2();

//sys_rwlock
extern int sys_rwlock_create(mem32_t rw_lock_id, mem_ptr_t<sys_rwlock_attribute_t> attr);
extern int sys_rwlock_destroy(u32 rw_lock_id);
extern int sys_rwlock_rlock(u32 rw_lock_id, u64 timeout);
extern int sys_rwlock_tryrlock(u32 rw_lock_id);
extern int sys_rwlock_runlock(u32 rw_lock_id);
extern int sys_rwlock_wlock(u32 rw_lock_id, u64 timeout);
extern int sys_rwlock_trywlock(u32 rw_lock_id);
extern int sys_rwlock_wunlock(u32 rw_lock_id);

//sys_rsx
extern int sys_rsx_device_open();
extern int sys_rsx_device_close();
extern int sys_rsx_memory_allocate();
extern int sys_rsx_memory_free();
extern int sys_rsx_context_allocate();
extern int sys_rsx_context_free();
extern int sys_rsx_context_iomap();
extern int sys_rsx_context_iounmap();
extern int sys_rsx_context_attribute(s32 context_id, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6);
extern int sys_rsx_device_map(mem32_t a1, mem32_t a2, u32 a3);
extern int sys_rsx_device_unmap();
extern int sys_rsx_attribute();

#define UNIMPLEMENTED_FUNC(module) module.Error("Unimplemented function: %s", __FUNCTION__)

#define SC_ARG_0 CPU.GPR[3]
#define SC_ARG_1 CPU.GPR[4]
#define SC_ARG_2 CPU.GPR[5]
#define SC_ARG_3 CPU.GPR[6]
#define SC_ARG_4 CPU.GPR[7]
#define SC_ARG_5 CPU.GPR[8]
#define SC_ARG_6 CPU.GPR[9]
#define SC_ARG_7 CPU.GPR[10]
#define SC_ARG_8 CPU.GPR[11]
#define SC_ARG_9 CPU.GPR[12]
#define SC_ARG_10 CPU.GPR[13]
#define SC_ARG_11 CPU.GPR[14]

#define SC_ARGS_1			SC_ARG_0
#define SC_ARGS_2 SC_ARGS_1,SC_ARG_1
#define SC_ARGS_3 SC_ARGS_2,SC_ARG_2
#define SC_ARGS_4 SC_ARGS_3,SC_ARG_3
#define SC_ARGS_5 SC_ARGS_4,SC_ARG_4
#define SC_ARGS_6 SC_ARGS_5,SC_ARG_5
#define SC_ARGS_7 SC_ARGS_6,SC_ARG_6
#define SC_ARGS_8 SC_ARGS_7,SC_ARG_7
#define SC_ARGS_9 SC_ARGS_8,SC_ARG_8
#define SC_ARGS_10 SC_ARGS_9,SC_ARG_9
#define SC_ARGS_11 SC_ARGS_10,SC_ARG_10
#define SC_ARGS_12 SC_ARGS_11,SC_ARG_11

extern bool dump_enable;

class SysCalls
{
public:
	static void DoSyscall(u32 code);
	static s64 DoFunc(const u32 id);
};

//extern SysCalls SysCallsManager;
