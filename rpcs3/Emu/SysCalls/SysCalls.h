#pragma once
#include "ErrorCodes.h"
#include "lv2/SC_FileSystem.h"
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
extern int sys_process_exit(int errorcode);
extern int sys_game_process_exitspawn(u32 path_addr, u32 argv_addr, u32 envp_addr,
								u32 data, u32 data_size, int prio, u64 flags );

//sys_event
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

//cellFs
extern int cellFsOpen(u32 path_addr, int flags, mem32_t fd, mem32_t arg, u64 size);
extern int cellFsRead(u32 fd, u32 buf_addr, u64 nbytes, mem64_t nread);
extern int cellFsWrite(u32 fd, u32 buf_addr, u64 nbytes, mem64_t nwrite);
extern int cellFsClose(u32 fd);
extern int cellFsOpendir(u32 path_addr, mem32_t fd);
extern int cellFsReaddir(u32 fd, u32 dir_addr, mem64_t nread);
extern int cellFsClosedir(u32 fd);
extern int cellFsStat(u32 path_addr, mem_struct_ptr_t<CellFsStat> sb);
extern int cellFsFstat(u32 fd, mem_struct_ptr_t<CellFsStat> sb);
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

//cellSysutil
extern int cellSysutilCheckCallback();
extern int cellSysutilRegisterCallback(int slot, u64 func_addr, u64 userdata);
extern int cellSysutilUnregisterCallback(int slot);

//cellMsgDialog
extern int cellMsgDialogOpen2(u32 type, u32 msgString_addr, u32 callback_addr, u32 userData, u32 extParam);

//cellPad
extern int cellPadInit(u32 max_connect);
extern int cellPadEnd();
extern int cellPadClearBuf(u32 port_no);
extern int cellPadGetData(u32 port_no, u32 data_addr);
extern int cellPadGetDataExtra(u32 port_no, u32 device_type_addr, u32 data_addr);
extern int cellPadSetActDirect(u32 port_no, u32 param_addr);
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

//cellResc
extern int cellRescSetSrc(const int idx, const u32 src_addr);
extern int cellRescSetBufferAddress(const u32 colorBuffers_addr, const u32 vertexArray_addr, const u32 fragmentShader_addr);

//sys_heap
extern int sys_heap_create_heap(const u32 heap_addr, const u32 start_addr, const u32 size);
extern int sys_heap_malloc(const u32 heap_addr, const u32 size);

//sys_spu
extern int sys_spu_image_open(u32 img_addr, u32 path_addr);
extern int sys_spu_thread_initialize(u32 thread_addr, u32 group, u32 spu_num, u32 img_addr, u32 attr_addr, u32 arg_addr);
extern int sys_spu_thread_set_argument(u32 id, u32 arg_addr);
extern int sys_spu_thread_group_start(u32 id);
extern int sys_spu_thread_group_create(u64 id_addr, u32 num, int prio, u64 attr_addr);
extern int sys_spu_thread_create(u64 thread_id_addr, u64 entry_addr, u64 arg, int prio, u32 stacksize, u64 flags, u64 threadname_addr);
extern int sys_raw_spu_create(u32 id_addr, u32 attr_addr);
extern int sys_spu_initialize(u32 max_usable_spu, u32 max_raw_spu);
extern int sys_spu_thread_write_ls(u32 id, u32 address, u64 value, u32 type);
extern int sys_spu_thread_read_ls(u32 id, u32 address, u32 value_addr, u32 type);
extern int sys_spu_thread_write_spu_mb(u32 id, u32 value);

//sys_time
extern int sys_time_get_current_time(u32 sec_addr, u32 nsec_addr);
extern s64 sys_time_get_system_time();
extern u64 sys_time_get_timebase_frequency();

#define UNIMPLEMENTED_FUNC(module) module.Error("Unimplemented function: "__FUNCTION__)

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
class PPUThread;

class SysCalls
{
	PPUThread& CPU;

public:
	//process
	int lv2ProcessGetPid(PPUThread& CPU);
	int lv2ProcessWaitForChild(PPUThread& CPU);
	int lv2ProcessGetStatus(PPUThread& CPU);
	int lv2ProcessDetachChild(PPUThread& CPU);
	int lv2ProcessGetNumberOfObject(PPUThread& CPU);
	int lv2ProcessGetId(PPUThread& CPU);
	int lv2ProcessGetPpid(PPUThread& CPU);
	int lv2ProcessKill(PPUThread& CPU);
	int lv2ProcessWaitForChild2(PPUThread& CPU);
	int lv2ProcessGetSdkVersion(PPUThread& CPU);

protected:
	SysCalls(PPUThread& cpu);
	~SysCalls();

public:
	void DoSyscall(u32 code);
	s64 DoFunc(const u32 id);
};

//extern SysCalls SysCallsManager;