#pragma once
#include "ErrorCodes.h"
#include "lv2/SC_FileSystem.h"
#include "lv2/SC_Memory.h"
#include "lv2/SC_Timer.h"
#include "lv2/SC_Rwlock.h"
#include "lv2/SC_SPU_Thread.h"
#include "lv2/SC_Lwmutex.h"
#include "lv2/SC_Lwcond.h"
#include "lv2/SC_Event_flag.h"
#include "lv2/SC_Condition.h"
#include "lv2/SC_Spinlock.h"
#include "Emu/event.h"
//#define SYSCALLS_DEBUG

#define declCPU PPUThread& CPU = GetCurrentPPUThread

class SysCallBase;

namespace detail{
	template<typename T> bool CheckId(u32 id, T*& data,const std::string &name)
	{
		ID* id_data;
		if(!CheckId(id, id_data,name)) return false;
		data = id_data->m_data->get<T>();
		return true;
	}

	template<> bool CheckId<ID>(u32 id, ID*& _id,const std::string &name);
}

class SysCallBase //Module
{
private:
	std::string m_module_name;
	//u32 m_id;

public:
	SysCallBase(const std::string& name/*, u32 id*/)
		: m_module_name(name)
		//, m_id(id)
	{
	}

	const std::string& GetName() const { return m_module_name; }

	void Log(const u32 id, std::string fmt, ...)
	{
		if(Ini.HLELogging.GetValue())
		{
		va_list list;
		va_start(list, fmt);
		ConLog.Write(GetName() + fmt::Format("[%d]: ", id) + fmt::FormatV(fmt, list));
		va_end(list);
		}
	}

	void Log(std::string fmt, ...)
	{
		if(Ini.HLELogging.GetValue())
		{
		va_list list;
		va_start(list, fmt);
		ConLog.Write(GetName() + ": " + fmt::FormatV(fmt, list));
		va_end(list);
		}
	}

	void Warning(const u32 id, std::string fmt, ...)
	{
//#ifdef SYSCALLS_DEBUG
		va_list list;
		va_start(list, fmt);
		ConLog.Warning(GetName() + fmt::Format("[%d] warning: ", id) + fmt::FormatV(fmt, list));
		va_end(list);
//#endif
	}

	void Warning(std::string fmt, ...)
	{
//#ifdef SYSCALLS_DEBUG
		va_list list;
		va_start(list, fmt);
		ConLog.Warning(GetName() + " warning: " + fmt::FormatV(fmt, list));
		va_end(list);
//#endif
	}

	void Error(const u32 id, std::string fmt, ...)
	{
		va_list list;
		va_start(list, fmt);
		ConLog.Error(GetName() + fmt::Format("[%d] error: ", id) + fmt::FormatV(fmt, list));
		va_end(list);
	}

	void Error(std::string fmt, ...)
	{
		va_list list;
		va_start(list, fmt);
		ConLog.Error(GetName() + " error: " + fmt::FormatV(fmt, list));
		va_end(list);
	}

	bool CheckId(u32 id) const
	{
		return Emu.GetIdManager().CheckID(id) && Emu.GetIdManager().GetID(id).m_name == GetName();
	}

	template<typename T> bool CheckId(u32 id, T*& data)
	{
		return detail::CheckId(id,data,GetName());
	}

	template<typename T>
	u32 GetNewId(T* data, u8 flags = 0)
	{
		return Emu.GetIdManager().GetNewID<T>(GetName(), data, flags);
	}
};

//process
extern int sys_process_getpid();
extern int sys_process_getppid();
extern int sys_process_get_number_of_object(u32 object, mem32_t nump);
extern int sys_process_get_id(u32 object, mem8_ptr_t buffer, u32 size, mem32_t set_size);
extern int sys_process_get_paramsfo(mem8_ptr_t buffer);
extern int sys_process_exit(int errorcode);
extern void sys_game_process_exitspawn(u32 path_addr, u32 argv_addr, u32 envp_addr,
								u32 data_addr, u32 data_size, u32 prio, u64 flags );
extern void sys_game_process_exitspawn2(u32 path_addr, u32 argv_addr, u32 envp_addr,
								u32 data_addr, u32 data_size, u32 prio, u64 flags);

//sys_event
extern int sys_event_queue_create(mem32_t equeue_id, mem_ptr_t<sys_event_queue_attr> attr, u64 event_queue_key, int size);
extern int sys_event_queue_destroy(u32 equeue_id, int mode);
extern int sys_event_queue_receive(u32 equeue_id, mem_ptr_t<sys_event_data> event, u64 timeout);
extern int sys_event_queue_tryreceive(u32 equeue_id, mem_ptr_t<sys_event_data> event_array, int size, mem32_t number);
extern int sys_event_queue_drain(u32 event_queue_id);
extern int sys_event_port_create(mem32_t eport_id, int port_type, u64 name);
extern int sys_event_port_destroy(u32 eport_id);
extern int sys_event_port_connect_local(u32 event_port_id, u32 event_queue_id);
extern int sys_event_port_disconnect(u32 eport_id);
extern int sys_event_port_send(u32 event_port_id, u64 data1, u64 data2, u64 data3);

//sys_event_flag
extern int sys_event_flag_create(mem32_t eflag_id, mem_ptr_t<sys_event_flag_attr> attr, u64 init);
extern int sys_event_flag_destroy(u32 eflag_id);
extern int sys_event_flag_wait(u32 eflag_id, u64 bitptn, u32 mode, mem64_t result, u64 timeout);
extern int sys_event_flag_trywait(u32 eflag_id, u64 bitptn, u32 mode, mem64_t result);
extern int sys_event_flag_set(u32 eflag_id, u64 bitptn);
extern int sys_event_flag_clear(u32 eflag_id, u64 bitptn);
extern int sys_event_flag_cancel(u32 eflag_id, mem32_t num);
extern int sys_event_flag_get(u32 eflag_id, mem64_t flags);

//sys_semaphore
extern int sys_semaphore_create(u32 sem_addr, u32 attr_addr, int initial_val, int max_val);
extern int sys_semaphore_destroy(u32 sem);
extern int sys_semaphore_wait(u32 sem, u64 timeout);
extern int sys_semaphore_trywait(u32 sem);
extern int sys_semaphore_post(u32 sem, int count);
extern int sys_semaphore_get_value(u32 sem, u32 count_addr);

//sys_lwcond
extern int sys_lwcond_create(mem_ptr_t<sys_lwcond_t> lwcond, mem_ptr_t<sys_lwmutex_t> lwmutex, mem_ptr_t<sys_lwcond_attribute_t> attr);
extern int sys_lwcond_destroy(mem_ptr_t<sys_lwcond_t> lwcond);
extern int sys_lwcond_signal(mem_ptr_t<sys_lwcond_t> lwcond);
extern int sys_lwcond_signal_all(mem_ptr_t<sys_lwcond_t> lwcond);
extern int sys_lwcond_signal_to(mem_ptr_t<sys_lwcond_t> lwcond, u32 ppu_thread_id);
extern int sys_lwcond_wait(mem_ptr_t<sys_lwcond_t> lwcond, u64 timeout);

//sys_lwmutex
extern int sys_lwmutex_create(mem_ptr_t<sys_lwmutex_t> lwmutex, mem_ptr_t<sys_lwmutex_attribute_t> attr);
extern int sys_lwmutex_destroy(mem_ptr_t<sys_lwmutex_t> lwmutex);
extern int sys_lwmutex_lock(mem_ptr_t<sys_lwmutex_t> lwmutex, u64 timeout);
extern int sys_lwmutex_trylock(mem_ptr_t<sys_lwmutex_t> lwmutex);
extern int sys_lwmutex_unlock(mem_ptr_t<sys_lwmutex_t> lwmutex);

//sys_cond
extern int sys_cond_create(mem32_t cond_id, u32 mutex_id, mem_ptr_t<sys_cond_attribute> attr);
extern int sys_cond_destroy(u32 cond_id);
extern int sys_cond_wait(u32 cond_id, u64 timeout);
extern int sys_cond_signal(u32 cond_id);
extern int sys_cond_signal_all(u32 cond_id);
extern int sys_cond_signal_to(u32 cond_id, u32 thread_id);

//sys_mutex
extern int sys_mutex_create(mem32_t mutex_id, mem_ptr_t<sys_mutex_attribute> attr);
extern int sys_mutex_destroy(u32 mutex_id);
extern int sys_mutex_lock(u32 mutex_id, u64 timeout);
extern int sys_mutex_trylock(u32 mutex_id);
extern int sys_mutex_unlock(u32 mutex_id);

//sys_rwlock
extern int sys_rwlock_create(mem32_t rw_lock_id, mem_ptr_t<sys_rwlock_attribute_t> attr);
extern int sys_rwlock_destroy(u32 rw_lock_id);
extern int sys_rwlock_rlock(u32 rw_lock_id, u64 timeout);
extern int sys_rwlock_tryrlock(u32 rw_lock_id);
extern int sys_rwlock_runlock(u32 rw_lock_id);
extern int sys_rwlock_wlock(u32 rw_lock_id, u64 timeout);
extern int sys_rwlock_trywlock(u32 rw_lock_id);
extern int sys_rwlock_wunlock(u32 rw_lock_id);

//sys_spinlock
extern void sys_spinlock_initialize(mem_ptr_t<spinlock> lock);
extern void sys_spinlock_lock(mem_ptr_t<spinlock> lock);
extern int sys_spinlock_trylock(mem_ptr_t<spinlock> lock);
extern void sys_spinlock_unlock(mem_ptr_t<spinlock> lock);

//ppu_thread
extern void sys_ppu_thread_exit(u64 errorcode);
extern int sys_ppu_thread_yield();
extern int sys_ppu_thread_join(u32 thread_id, mem64_t vptr);
extern int sys_ppu_thread_detach(u32 thread_id);
extern void sys_ppu_thread_get_join_state(u32 isjoinable_addr);
extern int sys_ppu_thread_set_priority(u32 thread_id, int prio);
extern int sys_ppu_thread_get_priority(u32 thread_id, u32 prio_addr);
extern int sys_ppu_thread_get_stack_information(u32 info_addr);
extern int sys_ppu_thread_stop(u32 thread_id);
extern int sys_ppu_thread_restart(u32 thread_id);
extern int sys_ppu_thread_create(u32 thread_id_addr, u32 entry, u64 arg, int prio, u32 stacksize, u64 flags, u32 threadname_addr);
extern void sys_ppu_thread_once(u32 once_ctrl_addr, u32 entry);
extern int sys_ppu_thread_get_id(const u32 id_addr);

//memory
extern int sys_memory_allocate(u32 size, u32 flags, u32 alloc_addr_addr);
extern int sys_memory_allocate_from_container(u32 size, u32 cid, u32 flags, u32 alloc_addr_addr);
extern int sys_memory_free(u32 start_addr);
extern int sys_memory_get_page_attribute(u32 addr, mem_ptr_t<sys_page_attr_t> attr);
extern int sys_memory_get_user_memory_size(u32 mem_info_addr);
extern int sys_memory_container_create(mem32_t cid, u32 yield_size);
extern int sys_memory_container_destroy(u32 cid);
extern int sys_memory_container_get_size(u32 mem_info_addr, u32 cid);
extern int sys_mmapper_allocate_address(u32 size, u64 flags, u32 alignment, u32 alloc_addr);
extern int sys_mmapper_allocate_fixed_address();
extern int sys_mmapper_allocate_memory(u32 size, u64 flags, mem32_t mem_id);
extern int sys_mmapper_allocate_memory_from_container(u32 size, u32 cid, u64 flags, mem32_t mem_id);
extern int sys_mmapper_change_address_access_right(u32 start_addr, u64 flags);
extern int sys_mmapper_free_address(u32 start_addr);
extern int sys_mmapper_free_memory(u32 mem_id);
extern int sys_mmapper_map_memory(u32 start_addr, u32 mem_id, u64 flags);
extern int sys_mmapper_search_and_map(u32 start_addr, u32 mem_id, u64 flags, u32 alloc_addr);
extern int sys_mmapper_unmap_memory(u32 start_addr, u32 mem_id_addr);
extern int sys_mmapper_enable_page_fault_notification(u32 start_addr, u32 q_id);

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
extern int cellFsReaddir(u32 fd, mem_ptr_t<CellFsDirent> dir, mem64_t nread);
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
extern int cellFsGetBlockSize(u32 path_addr, mem64_t sector_size, mem64_t block_size);
extern int cellFsGetFreeSize(u32 path_addr, mem32_t block_size, mem64_t block_count);
extern int cellFsGetDirectoryEntries(u32 fd, mem_ptr_t<CellFsDirectoryEntry> entries, u32 entries_size, mem32_t data_count);
extern int cellFsStReadInit(u32 fd, mem_ptr_t<CellFsRingBuffer> ringbuf);
extern int cellFsStReadFinish(u32 fd);
extern int cellFsStReadGetRingBuf(u32 fd, mem_ptr_t<CellFsRingBuffer> ringbuf);
extern int cellFsStReadGetStatus(u32 fd, mem64_t status);
extern int cellFsStReadGetRegid(u32 fd, mem64_t regid);
extern int cellFsStReadStart(u32 fd, u64 offset, u64 size);
extern int cellFsStReadStop(u32 fd);
extern int cellFsStRead(u32 fd, u32 buf_addr, u64 size, mem64_t rsize);
extern int cellFsStReadGetCurrentAddr(u32 fd, mem32_t addr_addr, mem64_t size);
extern int cellFsStReadPutCurrentAddr(u32 fd, u32 addr_addr, u64 size);
extern int cellFsStReadWait(u32 fd, u64 size);
extern int cellFsStReadWaitCallback(u32 fd, u64 size, mem_func_ptr_t<void (*)(int xfd, u64 xsize)> func);

//cellVideo
extern int cellVideoOutGetState(u32 videoOut, u32 deviceIndex, u32 state_addr);
extern int cellVideoOutGetResolution(u32 resolutionId, mem_ptr_t<CellVideoOutResolution> resolution);
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
extern int cellPadInfoPressMode(u32 port_no);
extern int cellPadInfoSensorMode(u32 port_no);
extern int cellPadSetPressMode(u32 port_no, u32 mode);
extern int cellPadSetSensorMode(u32 port_no, u32 mode);

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
extern int _sys_heap_memalign(u32 heap_id, u32 align, u32 size);

//sys_spu
extern int sys_spu_image_open(mem_ptr_t<sys_spu_image> img, u32 path_addr);
extern int sys_spu_thread_initialize(mem32_t thread, u32 group, u32 spu_num, mem_ptr_t<sys_spu_image> img, mem_ptr_t<sys_spu_thread_attribute> attr, mem_ptr_t<sys_spu_thread_argument> arg);
extern int sys_spu_thread_set_argument(u32 id, mem_ptr_t<sys_spu_thread_argument> arg);
extern int sys_spu_thread_group_destroy(u32 id);
extern int sys_spu_thread_group_start(u32 id);
extern int sys_spu_thread_group_suspend(u32 id);
extern int sys_spu_thread_group_resume(u32 id);
extern int sys_spu_thread_group_create(mem32_t id, u32 num, int prio, mem_ptr_t<sys_spu_thread_group_attribute> attr);
extern int sys_spu_thread_create(mem32_t thread_id, mem32_t entry, u64 arg, int prio, u32 stacksize, u64 flags, u32 threadname_addr);
extern int sys_spu_thread_group_join(u32 id, mem32_t cause, mem32_t status);
extern int sys_spu_thread_group_connect_event(u32 id, u32 eq, u32 et);
extern int sys_spu_thread_group_disconnect_event(u32 id, u32 et);
extern int sys_spu_thread_group_connect_event_all_threads(u32 id, u32 eq, u64 req, u32 spup_addr);
extern int sys_spu_thread_group_disconnect_event_all_threads(u32 id, u8 spup);
extern int sys_raw_spu_create(mem32_t id, u32 attr_addr);
extern int sys_spu_initialize(u32 max_usable_spu, u32 max_raw_spu);
extern int sys_spu_thread_write_ls(u32 id, u32 address, u64 value, u32 type);
extern int sys_spu_thread_read_ls(u32 id, u32 address, mem64_t value, u32 type);
extern int sys_spu_thread_write_spu_mb(u32 id, u32 value);
extern int sys_spu_thread_set_spu_cfg(u32 id, u64 value);
extern int sys_spu_thread_get_spu_cfg(u32 id, mem64_t value);
extern int sys_spu_thread_write_snr(u32 id, u32 number, u32 value);
extern int sys_spu_thread_connect_event(u32 id, u32 eq, u32 et, u8 spup);
extern int sys_spu_thread_disconnect_event(u32 id, u32 event_type, u8 spup);
extern int sys_spu_thread_bind_queue(u32 id, u32 spuq, u32 spuq_num);
extern int sys_spu_thread_unbind_queue(u32 id, u32 spuq_num);
extern int sys_spu_thread_get_exit_status(u32 id, mem32_t status);

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
/* // these definitions are wrong:
#define SC_ARG_8 CPU.GPR[11]
#define SC_ARG_9 CPU.GPR[12]
#define SC_ARG_10 CPU.GPR[13]
#define SC_ARG_11 CPU.GPR[14]
*/

#define SC_ARGS_1			SC_ARG_0
#define SC_ARGS_2 SC_ARGS_1,SC_ARG_1
#define SC_ARGS_3 SC_ARGS_2,SC_ARG_2
#define SC_ARGS_4 SC_ARGS_3,SC_ARG_3
#define SC_ARGS_5 SC_ARGS_4,SC_ARG_4
#define SC_ARGS_6 SC_ARGS_5,SC_ARG_5
#define SC_ARGS_7 SC_ARGS_6,SC_ARG_6
#define SC_ARGS_8 SC_ARGS_7,SC_ARG_7
/*
#define SC_ARGS_9 SC_ARGS_8,SC_ARG_8
#define SC_ARGS_10 SC_ARGS_9,SC_ARG_9
#define SC_ARGS_11 SC_ARGS_10,SC_ARG_10
#define SC_ARGS_12 SC_ARGS_11,SC_ARG_11
*/

extern bool dump_enable;

class SysCalls
{
public:
	static void DoSyscall(u32 code);
	static s64 DoFunc(const u32 id);
};

//extern SysCalls SysCallsManager;

void StaticAnalyse(void* ptr, u32 size, u32 base);
void StaticExecute(u32 code);
void StaticFinalize();

#define REG_SUB(module, group, name, ...) \
	static const u64 name ## _table[] = {__VA_ARGS__ , 0}; \
	module.AddFuncSub(group, name ## _table, #name, name)

#define REG_SUB_EMPTY(module, group, name,...) \
	static const u64 name ## _table[] = {0}; \
	module.AddFuncSub(group, name ## _table, #name, name)

extern u64 get_system_time();