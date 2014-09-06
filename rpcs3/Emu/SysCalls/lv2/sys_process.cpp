#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"

#include "sys_process.h"

SysCallBase sys_process("sys_process");

s32 process_getpid()
{
	// TODO: get current process id
	return 1;
}

s32 sys_process_getpid()
{
	sys_process.Log("sys_process_getpid() -> 1");
	return process_getpid();
}

s32 sys_process_getppid()
{
	sys_process.Todo("sys_process_getppid() -> 0");
	return 0;
}

s32 sys_process_exit(s32 errorcode)
{
	sys_process.Warning("sys_process_exit(%d)", errorcode);
	Emu.Pause();
	sys_process.Success("Process finished");
	CallAfter([]()
	{
		Emu.Stop();
	});
	return CELL_OK;
}

void sys_game_process_exitspawn(
			vm::ptr<const char> path,
			u32 argv_addr,
			u32 envp_addr,
			u32 data_addr,
			u32 data_size,
			u32 prio,
			u64 flags )
{
	sys_process.Todo("sys_game_process_exitspawn()");
	sys_process.Warning("path: %s", path.get_ptr());
	sys_process.Warning("argv: 0x%x", argv_addr);
	sys_process.Warning("envp: 0x%x", envp_addr);
	sys_process.Warning("data: 0x%x", data_addr);
	sys_process.Warning("data_size: 0x%x", data_size);
	sys_process.Warning("prio: %d", prio);
	sys_process.Warning("flags: %d", flags);

	std::string _path = path.get_ptr();
	std::vector<std::string> argv;
	std::vector<std::string> env;

	auto argvp = vm::ptr<vm::bptr<const char>>::make(argv_addr);
	while (argvp && *argvp)
	{
		argv.push_back(argvp[0].get_ptr());
		argvp++;
	}
	auto envp = vm::ptr<vm::bptr<const char>>::make(envp_addr);
	while (envp && *envp)
	{
		env.push_back(envp[0].get_ptr());
		envp++;
	}

	for (auto &arg : argv){
		sys_process.Log("argument: %s", arg.c_str());
	}
	for (auto &en : env){
		sys_process.Log("env_argument: %s", en.c_str());
	}
	//TODO: execute the file in <path> with the args in argv
	//and the environment parameters in envp and copy the data
	//from data_addr into the adress space of the new process
	//then kill the current process
	return;
}

void sys_game_process_exitspawn2(
			vm::ptr<const char> path,
			u32 argv_addr,
			u32 envp_addr,
			u32 data_addr,
			u32 data_size,
			u32 prio,
			u64 flags)
{
	sys_process.Todo("sys_game_process_exitspawn2");
	sys_process.Warning("path: %s", path.get_ptr());
	sys_process.Warning("argv: 0x%x", argv_addr);
	sys_process.Warning("envp: 0x%x", envp_addr);
	sys_process.Warning("data: 0x%x", data_addr);
	sys_process.Warning("data_size: 0x%x", data_size);
	sys_process.Warning("prio: %d", prio);
	sys_process.Warning("flags: %d", flags);

	std::string _path = path.get_ptr();
	std::vector<std::string> argv;
	std::vector<std::string> env;

	auto argvp = vm::ptr<vm::bptr<const char>>::make(argv_addr);
	while (argvp && *argvp)
	{
		argv.push_back(argvp[0].get_ptr());
		argvp++;
	}
	auto envp = vm::ptr<vm::bptr<const char>>::make(envp_addr);
	while (envp && *envp)
	{
		env.push_back(envp[0].get_ptr());
		envp++;
	}

	for (auto &arg : argv){
		sys_process.Log("argument: %s", arg.c_str());
	}
	for (auto &en : env){
		sys_process.Log("env_argument: %s", en.c_str());
	}
	//TODO: execute the file in <path> with the args in argv
	//and the environment parameters in envp and copy the data
	//from data_addr into the adress space of the new process
	//then kill the current process
	return;
}

s32 sys_process_get_number_of_object(u32 object, vm::ptr<be_t<u32>> nump)
{
	sys_process.Warning("sys_process_get_number_of_object(object=%d, nump_addr=0x%x)",
		object, nump.addr());

	switch(object)
	{
	case SYS_MEM_OBJECT:                  *nump = Emu.GetIdManager().GetTypeCount(TYPE_MEM);                 break;
	case SYS_MUTEX_OBJECT:                *nump = Emu.GetIdManager().GetTypeCount(TYPE_MUTEX);               break;
	case SYS_COND_OBJECT:                 *nump = Emu.GetIdManager().GetTypeCount(TYPE_COND);                break;
	case SYS_RWLOCK_OBJECT:               *nump = Emu.GetIdManager().GetTypeCount(TYPE_RWLOCK);              break;
	case SYS_INTR_TAG_OBJECT:             *nump = Emu.GetIdManager().GetTypeCount(TYPE_INTR_TAG);            break;
	case SYS_INTR_SERVICE_HANDLE_OBJECT:  *nump = Emu.GetIdManager().GetTypeCount(TYPE_INTR_SERVICE_HANDLE); break;
	case SYS_EVENT_QUEUE_OBJECT:          *nump = Emu.GetIdManager().GetTypeCount(TYPE_EVENT_QUEUE);         break;
	case SYS_EVENT_PORT_OBJECT:           *nump = Emu.GetIdManager().GetTypeCount(TYPE_EVENT_PORT);          break;
	case SYS_TRACE_OBJECT:                *nump = Emu.GetIdManager().GetTypeCount(TYPE_TRACE);               break;
	case SYS_SPUIMAGE_OBJECT:             *nump = Emu.GetIdManager().GetTypeCount(TYPE_SPUIMAGE);            break;
	case SYS_PRX_OBJECT:                  *nump = Emu.GetIdManager().GetTypeCount(TYPE_PRX);                 break;
	case SYS_SPUPORT_OBJECT:              *nump = Emu.GetIdManager().GetTypeCount(TYPE_SPUPORT);             break;
	case SYS_LWMUTEX_OBJECT:              *nump = Emu.GetIdManager().GetTypeCount(TYPE_LWMUTEX);             break;
	case SYS_TIMER_OBJECT:                *nump = Emu.GetIdManager().GetTypeCount(TYPE_TIMER);               break;
	case SYS_SEMAPHORE_OBJECT:            *nump = Emu.GetIdManager().GetTypeCount(TYPE_SEMAPHORE);           break;
	case SYS_LWCOND_OBJECT:               *nump = Emu.GetIdManager().GetTypeCount(TYPE_LWCOND);              break;
	case SYS_EVENT_FLAG_OBJECT:           *nump = Emu.GetIdManager().GetTypeCount(TYPE_EVENT_FLAG);          break;
	case SYS_FS_FD_OBJECT:
		*nump = Emu.GetIdManager().GetTypeCount(TYPE_FS_FILE) + Emu.GetIdManager().GetTypeCount(TYPE_FS_DIR);
		break;

	default:      
		return CELL_EINVAL;
	}

	return CELL_OK;
}

s32 sys_process_get_id(u32 object, vm::ptr<be_t<u32>> buffer, u32 size, vm::ptr<be_t<u32>> set_size)
{
	sys_process.Todo("sys_process_get_id(object=%d, buffer_addr=0x%x, size=%d, set_size_addr=0x%x)",
		object, buffer.addr(), size, set_size.addr());

	switch(object)
	{

#define ADD_OBJECTS(type) { \
	u32 i=0; \
	const auto& objects = Emu.GetIdManager().GetTypeIDs(type); \
	for(auto id=objects.begin(); i<size && id!=objects.end(); id++, i++) \
		buffer[i] = *id; \
	*set_size = i; \
	}

	case SYS_MEM_OBJECT:                  ADD_OBJECTS(TYPE_MEM);                 break;
	case SYS_MUTEX_OBJECT:                ADD_OBJECTS(TYPE_MUTEX);               break;
	case SYS_COND_OBJECT:                 ADD_OBJECTS(TYPE_COND);                break;
	case SYS_RWLOCK_OBJECT:               ADD_OBJECTS(TYPE_RWLOCK);              break;
	case SYS_INTR_TAG_OBJECT:             ADD_OBJECTS(TYPE_INTR_TAG);            break;
	case SYS_INTR_SERVICE_HANDLE_OBJECT:  ADD_OBJECTS(TYPE_INTR_SERVICE_HANDLE); break;
	case SYS_EVENT_QUEUE_OBJECT:          ADD_OBJECTS(TYPE_EVENT_QUEUE);         break;
	case SYS_EVENT_PORT_OBJECT:           ADD_OBJECTS(TYPE_EVENT_PORT);          break;
	case SYS_TRACE_OBJECT:                ADD_OBJECTS(TYPE_TRACE);               break;
	case SYS_SPUIMAGE_OBJECT:             ADD_OBJECTS(TYPE_SPUIMAGE);            break;
	case SYS_PRX_OBJECT:                  ADD_OBJECTS(TYPE_PRX);                 break;
	case SYS_SPUPORT_OBJECT:              ADD_OBJECTS(TYPE_SPUPORT);             break;
	case SYS_LWMUTEX_OBJECT:              ADD_OBJECTS(TYPE_LWMUTEX);             break;
	case SYS_TIMER_OBJECT:                ADD_OBJECTS(TYPE_TIMER);               break;
	case SYS_SEMAPHORE_OBJECT:            ADD_OBJECTS(TYPE_SEMAPHORE);           break;
	case SYS_FS_FD_OBJECT:                ADD_OBJECTS(TYPE_FS_FILE);/*TODO:DIR*/ break;
	case SYS_LWCOND_OBJECT:               ADD_OBJECTS(TYPE_LWCOND);              break;
	case SYS_EVENT_FLAG_OBJECT:           ADD_OBJECTS(TYPE_EVENT_FLAG);          break;

#undef ADD_OBJECTS

	default:      
		return CELL_EINVAL;
	}

	return CELL_OK;
}

s32 sys_process_get_paramsfo(vm::ptr<u8> buffer)
{
	sys_process.Todo("sys_process_get_paramsfo(buffer_addr=0x%x) -> CELL_ENOENT", buffer.addr());
	return CELL_ENOENT;

	/*//Before uncommenting this code, we should check if it is actually working.
	MemoryAllocator<be_t<u32>> fd;
	char filePath [] = "/app_home/../PARAM.SFO";
	if (!cellFsOpen(Memory.RealToVirtualAddr(filePath), 0, fd, NULL, 0))
		return CELL_ENOENT;

	MemoryAllocator<be_t<u64>> pos, nread;
	cellFsLseek(fd, 0, CELL_SEEK_SET, pos); //TODO: Move to the appropriate offset (probably 0x3F7)
	cellFsRead(fd, buffer.addr(), 40, nread); //WARNING: If offset==0x3F7: The file will end before the buffer (40 bytes) is filled!
	cellFsClose(fd);

	return CELL_OK;*/
}

s32 process_get_sdk_version(u32 pid, s32& ver)
{
	// TODO: get correct SDK version for selected pid
	ver = Emu.m_sdk_version;

	return CELL_OK;
}

s32 sys_process_get_sdk_version(u32 pid, vm::ptr<be_t<s32>> version)
{
	sys_process.Warning("sys_process_get_sdk_version(pid=%d, version_addr=0x%x)", pid, version.addr());

	s32 sdk_ver;
	s32 ret = process_get_sdk_version(pid, sdk_ver);
	if (ret != CELL_OK)
	{
		return ret; // error code
	}
	else
	{
		*version = sdk_ver;
		return CELL_OK;
	}
}

s32 sys_process_kill(u32 pid)
{
	sys_process.Todo("sys_process_kill(pid=%d)", pid);
	return CELL_OK;
}

s32 sys_process_wait_for_child(u32 pid, vm::ptr<be_t<u32>> status, u64 unk)
{
	sys_process.Todo("sys_process_wait_for_child(pid=%d, status_addr=0x%x, unk=0x%llx",
		pid, status.addr(), unk);
	return CELL_OK;
}

s32 sys_process_wait_for_child2(u64 unk1, u64 unk2, u64 unk3, u64 unk4, u64 unk5, u64 unk6)
{
	sys_process.Todo("sys_process_wait_for_child2(unk1=0x%llx, unk2=0x%llx, unk3=0x%llx, unk4=0x%llx, unk5=0x%llx, unk6=0x%llx)",
		unk1, unk2, unk3, unk4, unk5, unk6);
	return CELL_OK;
}

s32 sys_process_get_status(u64 unk)
{
	sys_process.Todo("sys_process_get_status(unk=0x%llx)", unk);
	//vm::write32(CPU.GPR[4], GetPPUThreadStatus(CPU));
	return CELL_OK;
}

s32 sys_process_detach_child(u64 unk)
{
	sys_process.Todo("sys_process_detach_child(unk=0x%llx)", unk);
	return CELL_OK;
}
