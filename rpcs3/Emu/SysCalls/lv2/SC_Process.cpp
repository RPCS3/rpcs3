#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"

SysCallBase sc_p("Process");

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

int sys_process_getpid()
{
	return 1;
}

int sys_process_getppid()
{
	sc_p.Warning("TODO: sys_process_getppid() returns 0");
	return 0;
}

int sys_process_exit(int errorcode)
{
	sc_p.Warning("sys_process_exit(%d)", errorcode);
	Emu.Pause(); // Emu.Stop() does crash
	ConLog.Success("Process finished");

	if (Ini.HLEExitOnStop.GetValue())
	{
		Ini.HLEExitOnStop.SetValue(false);
		// TODO: Find a way of calling Emu.Stop() and/or exiting RPCS3 (that is, TheApp->Exit()) without crashes
	}
	return CELL_OK;
}

void sys_game_process_exitspawn(
			u32 path_addr,
			u32 argv_addr,
			u32 envp_addr,
			u32 data_addr,
			u32 data_size,
			u32 prio,
			u64 flags )
{
	sc_p.Error("sys_game_process_exitspawn UNIMPLEMENTED");
	sc_p.Warning("path: %s", Memory.ReadString(path_addr).c_str());
	sc_p.Warning("argv: 0x%x", argv_addr);
	sc_p.Warning("envp: 0x%x", envp_addr);
	sc_p.Warning("data: 0x%x", data_addr);
	sc_p.Warning("data_size: 0x%x", data_size);
	sc_p.Warning("prio: %d", prio);
	sc_p.Warning("flags: %d", flags);

	std::string path = Memory.ReadString(path_addr);
	std::vector<std::string> argv;
	std::vector<std::string> env;

	mem_ptr_t<u32> argvp(argv_addr);
	while (argvp.GetAddr() && argvp.IsGood() && *argvp)
	{
		argv.push_back(Memory.ReadString(Memory.Read32(argvp.GetAddr())));
		argvp++;
	}
	mem_ptr_t<u32> envp(envp_addr);
	while (envp.GetAddr() && envp.IsGood() && *envp)
	{
		env.push_back(Memory.ReadString(Memory.Read32(envp.GetAddr())));
		envp++;
	}

	for (auto &arg : argv){
		sc_p.Log("argument: %s", arg.c_str());
	}
	for (auto &en : env){
		sc_p.Log("env_argument: %s", en.c_str());
	}
	//TODO: execute the file in <path> with the args in argv
	//and the environment parameters in envp and copy the data
	//from data_addr into the adress space of the new process
	//then kill the current process
	return;
}

void sys_game_process_exitspawn2(
			u32 path_addr,
			u32 argv_addr,
			u32 envp_addr,
			u32 data_addr,
			u32 data_size,
			u32 prio,
			u64 flags)
{
	sc_p.Error("sys_game_process_exitspawn2 UNIMPLEMENTED");
	sc_p.Warning("path: %s", Memory.ReadString(path_addr).c_str());
	sc_p.Warning("argv: 0x%x", argv_addr);
	sc_p.Warning("envp: 0x%x", envp_addr);
	sc_p.Warning("data: 0x%x", data_addr);
	sc_p.Warning("data_size: 0x%x", data_size);
	sc_p.Warning("prio: %d", prio);
	sc_p.Warning("flags: %d", flags);

	std::string path = Memory.ReadString(path_addr);
	std::vector<std::string> argv;
	std::vector<std::string> env;

	mem_ptr_t<u32> argvp(argv_addr);
	while (argvp.GetAddr() && argvp.IsGood() && *argvp)
	{
		argv.push_back(Memory.ReadString(Memory.Read32(argvp.GetAddr())));
		argvp++;
	}
	mem_ptr_t<u32> envp(envp_addr);
	while (envp.GetAddr() && envp.IsGood() && *envp)
	{
		env.push_back(Memory.ReadString(Memory.Read32(envp.GetAddr())));
		envp++;
	}

	for (auto &arg : argv){
		sc_p.Log("argument: %s", arg.c_str());
	}
	for (auto &en : env){
		sc_p.Log("env_argument: %s", en.c_str());
	}
	//TODO: execute the file in <path> with the args in argv
	//and the environment parameters in envp and copy the data
	//from data_addr into the adress space of the new process
	//then kill the current process
	return;
}


int sys_process_get_number_of_object(u32 object, mem32_t nump)
{
	sc_p.Warning("TODO: sys_process_get_number_of_object(object=%d, nump_addr=0x%x)",
		object, nump.GetAddr());
	
	switch(object)
	{
	case SYS_MEM_OBJECT:
		nump = 0;
		return CELL_OK;

	case SYS_MUTEX_OBJECT:
		nump = 0;
		return CELL_OK;

	case SYS_COND_OBJECT:
		nump = 0;
		return CELL_OK;

	case SYS_RWLOCK_OBJECT:
		nump = 0;
		return CELL_OK;

	case SYS_INTR_TAG_OBJECT:
		nump = 0;
		return CELL_OK;

	case SYS_INTR_SERVICE_HANDLE_OBJECT:
		nump = 0;
		return CELL_OK;

	case SYS_EVENT_QUEUE_OBJECT:
		nump = 0;
		return CELL_OK;

	case SYS_EVENT_PORT_OBJECT:
		nump = 0;
		return CELL_OK;

	case SYS_TRACE_OBJECT:
		nump = 0;
		return CELL_OK;

	case SYS_SPUIMAGE_OBJECT:
		nump = 0;
		return CELL_OK;

	case SYS_PRX_OBJECT:
		nump = 0;
		return CELL_OK;

	case SYS_SPUPORT_OBJECT:
		nump = 0;
		return CELL_OK;

	case SYS_LWMUTEX_OBJECT:
		nump = 0;
		return CELL_OK;

	case SYS_TIMER_OBJECT:
		nump = 0;
		return CELL_OK;

	case SYS_SEMAPHORE_OBJECT:
		nump = 0;
		return CELL_OK;

	case SYS_FS_FD_OBJECT:
		nump = 0;
		return CELL_OK;

	case SYS_LWCOND_OBJECT:
		nump = 0;
		return CELL_OK;

	default:
		return CELL_EINVAL;
	}
}

int sys_process_get_id(u32 object, mem8_ptr_t buffer, u32 size, mem32_t set_size)
{
	sc_p.Warning("TODO: sys_process_get_id(object=%d, buffer_addr=0x%x, size=%d, set_size_addr=0x%x)",
		object, buffer.GetAddr(), size, set_size.GetAddr());

	switch(object)
	{
	case SYS_MEM_OBJECT:
		set_size = 0;
		return CELL_OK;

	case SYS_MUTEX_OBJECT:
		set_size = 0;
		return CELL_OK;

	case SYS_COND_OBJECT:
		set_size = 0;
		return CELL_OK;

	case SYS_RWLOCK_OBJECT:
		set_size = 0;
		return CELL_OK;

	case SYS_INTR_TAG_OBJECT:
		set_size = 0;
		return CELL_OK;

	case SYS_INTR_SERVICE_HANDLE_OBJECT:
		set_size = 0;
		return CELL_OK;

	case SYS_EVENT_QUEUE_OBJECT:
		set_size = 0;
		return CELL_OK;

	case SYS_EVENT_PORT_OBJECT:
		set_size = 0;
		return CELL_OK;

	case SYS_TRACE_OBJECT:
		set_size = 0;
		return CELL_OK;

	case SYS_SPUIMAGE_OBJECT:
		set_size = 0;
		return CELL_OK;

	case SYS_PRX_OBJECT:
		set_size = 0;
		return CELL_OK;

	case SYS_SPUPORT_OBJECT:
		set_size = 0;
		return CELL_OK;

	case SYS_LWMUTEX_OBJECT:
		set_size = 0;
		return CELL_OK;

	case SYS_TIMER_OBJECT:
		set_size = 0;
		return CELL_OK;

	case SYS_SEMAPHORE_OBJECT:
		set_size = 0;
		return CELL_OK;

	case SYS_FS_FD_OBJECT:
		set_size = 0;
		return CELL_OK;

	case SYS_LWCOND_OBJECT:
		set_size = 0;
		return CELL_OK;

	case SYS_EVENT_FLAG_OBJECT:
		set_size = 0;
		return CELL_OK;

	default:
		return CELL_EINVAL;
	}
}

int sys_process_get_paramsfo(mem8_ptr_t buffer)
{
	sc_p.Warning("TODO: sys_process_get_paramsfo(buffer_addr=0x%x) returns CELL_ENOENT", buffer.GetAddr());
	return CELL_ENOENT;

	/*//Before uncommenting this code, we should check if it is actually working.
	MemoryAllocator<be_t<u32>> fd;
	char filePath [] = "/app_home/../PARAM.SFO";
	if (!cellFsOpen(Memory.RealToVirtualAddr(filePath), 0, fd, NULL, 0))
		return CELL_ENOENT;

	MemoryAllocator<be_t<u64>> pos, nread;
	cellFsLseek(fd, 0, CELL_SEEK_SET, pos); //TODO: Move to the appropriate offset (probably 0x3F7)
	cellFsRead(fd, buffer.GetAddr(), 40, nread); //WARNING: If offset==0x3F7: The file will end before the buffer (40 bytes) is filled!
	cellFsClose(fd);

	return CELL_OK;*/
}

/*
int SysCalls::lv2ProcessWaitForChild(PPUThread& CPU)
{
	ConLog.Warning("lv2ProcessWaitForChild");
	return CELL_OK;
}
int SysCalls::lv2ProcessGetStatus(PPUThread& CPU)
{
	ConLog.Warning("lv2ProcessGetStatus");
	//Memory.Write32(CPU.GPR[4], GetPPUThreadStatus(CPU));
	return CELL_OK;
}
int SysCalls::lv2ProcessDetachChild(PPUThread& CPU)
{
	ConLog.Warning("lv2ProcessDetachChild");
	return CELL_OK;
}
int SysCalls::lv2ProcessGetNumberOfObject(PPUThread& CPU)
{
	ConLog.Warning("lv2ProcessGetNumberOfObject");
	Memory.Write32(CPU.GPR[4], 1);
	return CELL_OK;
}
int SysCalls::lv2ProcessGetId(PPUThread& CPU)
{
	ConLog.Warning("lv2ProcessGetId");
	Memory.Write32(CPU.GPR[4], CPU.GetId());
	return CELL_OK;
}
int SysCalls::lv2ProcessGetPpid(PPUThread& CPU)
{
	ConLog.Warning("lv2ProcessGetPpid");
	Memory.Write32(CPU.GPR[4], CPU.GetId());
	return CELL_OK;
}
int SysCalls::lv2ProcessKill(PPUThread& CPU)
{
	ConLog.Warning("lv2ProcessKill[pid: 0x%llx]", CPU.GPR[3]);
	CPU.Close();
	return CELL_OK;
}
int SysCalls::lv2ProcessWaitForChild2(PPUThread& CPU)
{
	ConLog.Warning("lv2ProcessWaitForChild2[r3: 0x%llx, r4: 0x%llx, r5: 0x%llx, r6: 0x%llx, r7: 0x%llx, r8: 0x%llx]",
		CPU.GPR[3], CPU.GPR[4], CPU.GPR[5], CPU.GPR[6], CPU.GPR[7], CPU.GPR[8]);
	return CELL_OK;
}
int SysCalls::lv2ProcessGetSdkVersion(PPUThread& CPU)
{
	ConLog.Warning("lv2ProcessGetSdkVersion[r3: 0x%llx, r4: 0x%llx]", CPU.GPR[3], CPU.GPR[4]);
	CPU.GPR[4] = 0x360001; //TODO
	return CELL_OK;
}
*/
