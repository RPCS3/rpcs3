#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"
#include "sys_process.h"
#include "rpcs3.h"

SysCallBase sc_p("Process");

sysProcessObjects_t procObjects;

s32 process_getpid()
{
	// TODO: get current process id
	return 1;
}

s32 sys_process_getpid()
{
	sc_p.Log("sys_process_getpid() -> 1");
	return process_getpid();
}

s32 sys_process_getppid()
{
	sc_p.Todo("sys_process_getppid() -> 0");
	return 0;
}

s32 sys_process_exit(s32 errorcode)
{
	sc_p.Warning("sys_process_exit(%d)", errorcode);
	Emu.Pause();
	LOG_SUCCESS(HLE, "Process finished");
	wxGetApp().CallAfter([]()
	{
		Emu.Stop();
		if (Ini.HLEExitOnStop.GetValue())
		{
			wxGetApp().Exit();
		}
	});
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
	sc_p.Todo("sys_game_process_exitspawn()");
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
	while (argvp.GetAddr() && *argvp)
	{
		argv.push_back(Memory.ReadString(Memory.Read32(argvp.GetAddr())));
		argvp++;
	}
	mem_ptr_t<u32> envp(envp_addr);
	while (envp.GetAddr() && *envp)
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
	sc_p.Todo("sys_game_process_exitspawn2");
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
	while (argvp.GetAddr() && *argvp)
	{
		argv.push_back(Memory.ReadString(Memory.Read32(argvp.GetAddr())));
		argvp++;
	}
	mem_ptr_t<u32> envp(envp_addr);
	while (envp.GetAddr() && *envp)
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

s32 sys_process_get_number_of_object(u32 object, mem32_t nump)
{
	sc_p.Warning("sys_process_get_number_of_object(object=%d, nump_addr=0x%x)",
		object, nump.GetAddr());

	switch(object)
	{
	case SYS_MEM_OBJECT:                  nump = procObjects.mem_objects.size();                 break;
	case SYS_MUTEX_OBJECT:                nump = procObjects.mutex_objects.size();               break;
	case SYS_COND_OBJECT:                 nump = procObjects.cond_objects.size();                break;
	case SYS_RWLOCK_OBJECT:               nump = procObjects.rwlock_objects.size();              break;
	case SYS_INTR_TAG_OBJECT:             nump = procObjects.intr_tag_objects.size();            break;
	case SYS_INTR_SERVICE_HANDLE_OBJECT:  nump = procObjects.intr_service_handle_objects.size(); break;
	case SYS_EVENT_QUEUE_OBJECT:          nump = procObjects.event_queue_objects.size();         break;
	case SYS_EVENT_PORT_OBJECT:           nump = procObjects.event_port_objects.size();          break;
	case SYS_TRACE_OBJECT:                nump = procObjects.trace_objects.size();               break;
	case SYS_SPUIMAGE_OBJECT:             nump = procObjects.spuimage_objects.size();            break;
	case SYS_PRX_OBJECT:                  nump = procObjects.prx_objects.size();                 break;
	case SYS_SPUPORT_OBJECT:              nump = procObjects.spuport_objects.size();             break;
	case SYS_LWMUTEX_OBJECT:              nump = procObjects.lwmutex_objects.size();             break;
	case SYS_TIMER_OBJECT:                nump = procObjects.timer_objects.size();               break;
	case SYS_SEMAPHORE_OBJECT:            nump = procObjects.semaphore_objects.size();           break;
	case SYS_FS_FD_OBJECT:                nump = procObjects.fs_fd_objects.size();               break;
	case SYS_LWCOND_OBJECT:               nump = procObjects.lwcond_objects.size();              break;

	default:      
		return CELL_EINVAL;
	}

	return CELL_OK;
}

s32 sys_process_get_id(u32 object, mem32_ptr_t buffer, u32 size, mem32_t set_size)
{
	sc_p.Todo("sys_process_get_id(object=%d, buffer_addr=0x%x, size=%d, set_size_addr=0x%x)",
		object, buffer.GetAddr(), size, set_size.GetAddr());

	switch(object)
	{

#define ADD_OBJECTS(objects) { \
	u32 i=0; \
	for(auto id=objects.begin(); i<size && id!=objects.end(); id++, i++) \
		buffer[i] = *id; \
	set_size = i; \
	}

	case SYS_MEM_OBJECT:                  ADD_OBJECTS(procObjects.mem_objects);                 break;
	case SYS_MUTEX_OBJECT:                ADD_OBJECTS(procObjects.mutex_objects);               break;
	case SYS_COND_OBJECT:                 ADD_OBJECTS(procObjects.cond_objects);                break;
	case SYS_RWLOCK_OBJECT:               ADD_OBJECTS(procObjects.rwlock_objects);              break;
	case SYS_INTR_TAG_OBJECT:             ADD_OBJECTS(procObjects.intr_tag_objects);            break;
	case SYS_INTR_SERVICE_HANDLE_OBJECT:  ADD_OBJECTS(procObjects.intr_service_handle_objects); break;
	case SYS_EVENT_QUEUE_OBJECT:          ADD_OBJECTS(procObjects.event_queue_objects);         break;
	case SYS_EVENT_PORT_OBJECT:           ADD_OBJECTS(procObjects.event_port_objects);          break;
	case SYS_TRACE_OBJECT:                ADD_OBJECTS(procObjects.trace_objects);               break;
	case SYS_SPUIMAGE_OBJECT:             ADD_OBJECTS(procObjects.spuimage_objects);            break;
	case SYS_PRX_OBJECT:                  ADD_OBJECTS(procObjects.prx_objects);                 break;
	case SYS_SPUPORT_OBJECT:              ADD_OBJECTS(procObjects.spuport_objects);             break;
	case SYS_LWMUTEX_OBJECT:              ADD_OBJECTS(procObjects.lwmutex_objects);             break;
	case SYS_TIMER_OBJECT:                ADD_OBJECTS(procObjects.timer_objects);               break;
	case SYS_SEMAPHORE_OBJECT:            ADD_OBJECTS(procObjects.semaphore_objects);           break;
	case SYS_FS_FD_OBJECT:                ADD_OBJECTS(procObjects.fs_fd_objects);               break;
	case SYS_LWCOND_OBJECT:               ADD_OBJECTS(procObjects.lwcond_objects);              break;
	case SYS_EVENT_FLAG_OBJECT:           ADD_OBJECTS(procObjects.event_flag_objects);          break;

#undef ADD_OBJECTS

	default:      
		return CELL_EINVAL;
	}

	return CELL_OK;
}

s32 sys_process_get_paramsfo(mem8_ptr_t buffer)
{
	sc_p.Todo("sys_process_get_paramsfo(buffer_addr=0x%x) -> CELL_ENOENT", buffer.GetAddr());
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

s32 process_get_sdk_version(u32 pid, s32& ver)
{
	// TODO: get correct SDK version for selected pid
	ver = Emu.m_sdk_version;

	return CELL_OK;
}

s32 sys_process_get_sdk_version(u32 pid, mem32_t version)
{
	sc_p.Warning("sys_process_get_sdk_version(pid=%d, version_addr=0x%x)", pid, version.GetAddr());

	s32 sdk_ver;
	s32 ret = process_get_sdk_version(pid, sdk_ver);
	if (ret != CELL_OK)
	{
		return ret; // error code
	}
	else
	{
		version = sdk_ver;
		return CELL_OK;
	}
}

s32 sys_process_kill(u32 pid)
{
	sc_p.Todo("sys_process_kill(pid=%d)", pid);
	return CELL_OK;
}

s32 sys_process_wait_for_child(u32 pid, mem32_t status, u64 unk)
{
	sc_p.Todo("sys_process_wait_for_child(pid=%d, status_addr=0x%x, unk=0x%llx",
		pid, status.GetAddr(), unk);
	return CELL_OK;
}

s32 sys_process_wait_for_child2(u64 unk1, u64 unk2, u64 unk3, u64 unk4, u64 unk5, u64 unk6)
{
	sc_p.Todo("sys_process_wait_for_child2(unk1=0x%llx, unk2=0x%llx, unk3=0x%llx, unk4=0x%llx, unk5=0x%llx, unk6=0x%llx)",
		unk1, unk2, unk3, unk4, unk5, unk6);
	return CELL_OK;
}

s32 sys_process_get_status(u64 unk)
{
	sc_p.Todo("sys_process_get_status(unk=0x%llx)", unk);
	//Memory.Write32(CPU.GPR[4], GetPPUThreadStatus(CPU));
	return CELL_OK;
}

s32 sys_process_detach_child(u64 unk)
{
	sc_p.Todo("sys_process_detach_child(unk=0x%llx)", unk);
	return CELL_OK;
}
