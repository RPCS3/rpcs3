#pragma once
#include <Emu/Cell/PPUThread.h>
//#include <Emu/Cell/SPUThread.h>
#include "ErrorCodes.h"

//#define SYSCALLS_DEBUG

class SysCallBase
{
private:
	wxString m_module_name;

public:
	SysCallBase(const wxString& name) : m_module_name(name)
	{
	}

	wxString GetName() { return m_module_name; }

	void Log(const u32 id, wxString fmt, ...)
	{
#ifdef SYSCALLS_DEBUG
		va_list list;
		va_start(list, fmt);
		ConLog.Write(GetName() + wxString::Format("[%d]: ", id) + wxString::FormatV(fmt, list));
		va_end(list);
#endif
	}

	void Log(wxString fmt, ...)
	{
#ifdef SYSCALLS_DEBUG
		va_list list;
		va_start(list, fmt);
		ConLog.Write(GetName() + ": " + wxString::FormatV(fmt, list));
		va_end(list);
#endif
	}

	void Warning(const u32 id, wxString fmt, ...)
	{
#ifdef SYSCALLS_DEBUG
		va_list list;
		va_start(list, fmt);
		ConLog.Warning(GetName() + wxString::Format("[%d] warning: ", id) + wxString::FormatV(fmt, list));
		va_end(list);
#endif
	}

	void Warning(wxString fmt, ...)
	{
#ifdef SYSCALLS_DEBUG
		va_list list;
		va_start(list, fmt);
		ConLog.Warning(GetName() + wxString::Format(" warning: ") + wxString::FormatV(fmt, list));
		va_end(list);
#endif
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
		ConLog.Error(GetName() + wxString::Format(" error: ") + wxString::FormatV(fmt, list));
		va_end(list);
	}
};

static bool CmpPath(const wxString& path1, const wxString& path2)
{
	return path1.Len() >= path2.Len() && path1(0, path2.Len()).CmpNoCase(path2) == 0;
}

static wxString GetWinPath(const wxString& path)
{
	if(!CmpPath(path, "/") && CmpPath(path(1, 1), ":")) return path;

	wxString ppath = wxFileName(Emu.m_path).GetPath() + '/' + wxFileName(path).GetFullName();
	if(wxFileExists(ppath)) return ppath;

	if (CmpPath(path, "/dev_hdd0/") ||
		CmpPath(path, "/dev_hdd1/") ||
		CmpPath(path, "/dev_bdvd/") ||
		CmpPath(path, "/dev_usb001/") ||
		CmpPath(path, "/ps3_home/") ||
		CmpPath(path, "/app_home/") ||
		CmpPath(path, "/dev_flash/") ||
		CmpPath(path, "/dev_flash2/") ||
		CmpPath(path, "/dev_flash3/")
		) return wxGetCwd() + path;

	return wxFileName(Emu.m_path).GetPath() + (path[0] == '/' ? path : "/" + path);
}

//process
extern int sys_process_getpid();
extern int sys_game_process_exitspawn(u32 path_addr, u32 argv_addr, u32 envp_addr,
								u32 data, u32 data_size, int prio, u64 flags );

//memory
extern int sys_memory_container_create(u32 cid_addr, u32 yield_size);
extern int sys_memory_container_destroy(u32 cid);
extern int sys_memory_allocate(u32 size, u32 flags, u32 alloc_addr_addr);
extern int sys_memory_get_user_memory_size(u32 mem_info_addr);

//cellFs
extern int cellFsOpen(const u32 path_addr, const int flags, const u32 fd_addr, const u32 arg_addr, const u64 size);
extern int cellFsRead(const u32 fd, const u32 buf_addr, const u64 nbytes, const u32 nread_addr);
extern int cellFsWrite(const u32 fd, const u32 buf_addr, const u64 nbytes, const u32 nwrite_addr);
extern int cellFsClose(const u32 fd);
extern int cellFsOpendir(const u32 path_addr, const u32 fd_addr);
extern int cellFsReaddir(const u32 fd, const u32 dir_addr, const u32 nread_addr);
extern int cellFsClosedir(const u32 fd);
extern int cellFsStat(const u32 path_addr, const u32 sb_addr);
extern int cellFsFstat(const u32 fd, const u32 sb_addr);
extern int cellFsMkdir(const u32 path_addr, const u32 mode);
extern int cellFsRename(const u32 from_addr, const u32 to_addr);
extern int cellFsRmdir(const u32 path_addr);
extern int cellFsUnlink(const u32 path_addr);
extern int cellFsLseek(const u32 fd, const s64 offset, const u32 whence, const u32 pos_addr);

//cellVideo
extern int cellVideoOutGetState(u32 videoOut, u32 deviceIndex, u32 state_addr);
extern int cellVideoOutGetResolution(u32 resolutionId, u32 resolution_addr);

//cellPad
extern int cellPadInit(u32 max_connect);
extern int cellPadEnd();
extern int cellPadClearBuf(u32 port_no);
extern int cellPadGetData(u32 port_no, u32 data_addr);
extern int cellPadGetDataExtra(u32 port_no, u32 device_type_addr, u32 data_addr);
extern int cellPadSetActDirect(u32 port_no, u32 param_addr);
extern int cellPadGetInfo2(u32 info_addr);
extern int cellPadSetPortSetting(u32 port_no, u32 port_setting);

//cellGcm
extern int cellGcmInit(const u32 context_addr, const u32 cmdSize, const u32 ioSize, const u32 ioAddress);
extern int cellGcmGetConfiguration(const u32 config_addr);
extern int cellGcmAddressToOffset(const u32 address, const u32 offset_addr);
extern int cellGcmSetDisplayBuffer(const u8 id, const u32 offset, const u32 pitch, const u32 width, const u32 height);
extern u32 cellGcmGetLabelAddress(const u32 index);
extern u32 cellGcmGetControlRegister();
extern int cellGcmFlush(const u32 ctx, const u8 id);
extern int cellGcmSetTile(const u8 index, const u8 location, const u32 offset, const u32 size, const u32 pitch, const u8 comp, const u16 base, const u8 bank);
extern int cellGcmGetFlipStatus();
extern int cellGcmResetFlipStatus();
extern u32 cellGcmGetTiledPitchSize(const u32 size);

//cellResc
extern int cellRescSetSrc(const int idx, const u32 src_addr);
extern int cellRescSetBufferAddress(const u32 colorBuffers_addr, const u32 vertexArray_addr, const u32 fragmentShader_addr);

//sys_heap
extern int sys_heap_create_heap(const u32 heap_addr, const u32 start_addr, const u32 size);
extern int sys_heap_malloc(const u32 heap_addr, const u32 size);

//sys_spu
extern int sys_spu_thread_group_create(u64 id_addr, u32 num, int prio, u64 attr_addr);
extern int sys_spu_thread_create(u64 thread_id_addr, u64 entry_addr, u64 arg, int prio, u32 stacksize, u64 flags, u64 threadname_addr);
extern int sys_raw_spu_create(u32 id_addr, u32 attr_addr);
extern int sys_spu_initialize(u32 max_usable_spu, u32 max_raw_spu);

//sys_time
extern int sys_time_get_current_time(u32 sec_addr, u32 nsec_addr);
extern s64 sys_time_get_system_time();
extern u64 sys_time_get_timebase_frequency();

#define SC_ARGS_1 CPU.GPR[3]
#define SC_ARGS_2 SC_ARGS_1,CPU.GPR[4]
#define SC_ARGS_3 SC_ARGS_2,CPU.GPR[5]
#define SC_ARGS_4 SC_ARGS_3,CPU.GPR[6]
#define SC_ARGS_5 SC_ARGS_4,CPU.GPR[7]
#define SC_ARGS_6 SC_ARGS_5,CPU.GPR[8]
#define SC_ARGS_7 SC_ARGS_6,CPU.GPR[9]
#define SC_ARGS_8 SC_ARGS_7,CPU.GPR[10]

extern bool dump_enable;

class SysCalls
{
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
	int lv2ProcessExit(PPUThread& CPU);
	int lv2ProcessWaitForChild2(PPUThread& CPU);
	int lv2ProcessGetSdkVersion(PPUThread& CPU);

	//ppu thread
	int lv2PPUThreadCreate(PPUThread& CPU);
	int lv2PPUThreadExit(PPUThread& CPU);
	int lv2PPUThreadYield(PPUThread& CPU);
	int lv2PPUThreadJoin(PPUThread& CPU);
	int lv2PPUThreadDetach(PPUThread& CPU);
	int lv2PPUThreadGetJoinState(PPUThread& CPU);
	int lv2PPUThreadSetPriority(PPUThread& CPU);
	int lv2PPUThreadGetPriority(PPUThread& CPU);
	int lv2PPUThreadGetStackInformation(PPUThread& CPU);
	int lv2PPUThreadRename(PPUThread& CPU);
	int lv2PPUThreadRecoverPageFault(PPUThread& CPU);
	int lv2PPUThreadGetPageFaultContext(PPUThread& CPU);
	int lv2PPUThreadGetId(PPUThread& CPU);

	//lwmutex
	int Lv2LwmutexCreate(PPUThread& CPU);
	int Lv2LwmutexDestroy(PPUThread& CPU);
	int Lv2LwmutexLock(PPUThread& CPU);
	int Lv2LwmutexTrylock(PPUThread& CPU);
	int Lv2LwmutexUnlock(PPUThread& CPU);

	//tty
	int lv2TtyRead(PPUThread& CPU);
	int lv2TtyWrite(PPUThread& CPU);

public:
	SysCalls()// : CPU(cpu)
	{
	}

	~SysCalls()
	{
		Close();
	}

	void Close()
	{
	}

	s64 DoSyscall(u32 code, PPUThread& CPU)
	{
		switch(code)
		{
			//=== lv2 ===
			//process
			case 1: return sys_process_getpid();
			case 2: return lv2ProcessWaitForChild(CPU);
			case 3: return lv2ProcessExit(CPU);
			case 4: return lv2ProcessGetStatus(CPU);
			case 5: return lv2ProcessDetachChild(CPU);
			case 12: return lv2ProcessGetNumberOfObject(CPU);
			case 13: return lv2ProcessGetId(CPU);
			case 18: return lv2ProcessGetPpid(CPU);
			case 19: return lv2ProcessKill(CPU);
			case 22: return lv2ProcessExit(CPU);
			case 23: return lv2ProcessWaitForChild2(CPU);
			case 25: return lv2ProcessGetSdkVersion(CPU);
			//ppu thread
			//case ?: return lv2PPUThreadCreate(CPU);
			//case ?: return lv2PPUThreadExit(CPU);
			case 43: return lv2PPUThreadYield(CPU);
			case 44: return lv2PPUThreadJoin(CPU);
			case 45: return lv2PPUThreadDetach(CPU);
			case 46: return lv2PPUThreadGetJoinState(CPU);
			case 47: return lv2PPUThreadSetPriority(CPU);
			case 48: return lv2PPUThreadGetPriority(CPU);
			case 49: return lv2PPUThreadGetStackInformation(CPU);
			case 56: return lv2PPUThreadRename(CPU);
			case 57: return lv2PPUThreadRecoverPageFault(CPU);
			case 58: return lv2PPUThreadGetPageFaultContext(CPU);
			//case ?: return lv2PPUThreadGetId(CPU);
			//lwmutex
			case 95: return Lv2LwmutexCreate(CPU);
			case 96: return Lv2LwmutexDestroy(CPU);
			case 97: return Lv2LwmutexLock(CPU);
			case 98: return Lv2LwmutexTrylock(CPU);
			case 99: return Lv2LwmutexUnlock(CPU);
			//timer
			case 141:
			case 142:
				//wxSleep(Emu.GetCPU().GetThreads().GetCount() > 1 ? 1 : /*SC_ARGS_1*/1);
			return 0;
			//time
			case 145: return sys_time_get_current_time(SC_ARGS_2);
			case 146: return sys_time_get_system_time();
			case 147: return sys_time_get_timebase_frequency();
			//sys_spu
			case 160: return sys_raw_spu_create(SC_ARGS_2);
			case 169: return sys_spu_initialize(SC_ARGS_2);
			case 170: return sys_spu_thread_group_create(SC_ARGS_4);
			//memory
			case 324: return sys_memory_container_create(SC_ARGS_2);
			case 325: return sys_memory_container_destroy(SC_ARGS_1);
			case 348: return sys_memory_allocate(SC_ARGS_3);
			case 352: return sys_memory_get_user_memory_size(SC_ARGS_1);
			//tty
			case 402: return lv2TtyRead(CPU);
			case 403: return lv2TtyWrite(CPU);
			//file system
			case 801: return cellFsOpen(SC_ARGS_5);
			case 802: return cellFsRead(SC_ARGS_4);
			case 803: return cellFsWrite(SC_ARGS_4);
			case 804: return cellFsClose(SC_ARGS_1);
			case 805: return cellFsOpendir(SC_ARGS_2);
			case 806: return cellFsReaddir(SC_ARGS_3);
			case 807: return cellFsClosedir(SC_ARGS_1);
			case 809: return cellFsFstat(SC_ARGS_2);
			case 811: return cellFsMkdir(SC_ARGS_2);
			case 812: return cellFsRename(SC_ARGS_2);
			case 813: return cellFsRmdir(SC_ARGS_1);
			case 818: return cellFsLseek(SC_ARGS_4);
			case 988:
				ConLog.Warning("SysCall 988! r3: 0x%llx, r4: 0x%llx, pc: 0x%llx",
					CPU.GPR[3], CPU.GPR[4], CPU.PC);
			return 0;

			case 999:
				dump_enable = !dump_enable;
				ConLog.Warning("Dump %s", dump_enable ? "enabled" : "disabled");
			return 0;
		}

		ConLog.Error("Unknown syscall: %d - %08x", code, code);
		return 0;
	}
	
	s64 DoFunc(const u32 id, PPUThread& CPU);
};

extern SysCalls SysCallsManager;