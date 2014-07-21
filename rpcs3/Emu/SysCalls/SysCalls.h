#pragma once
#include "ErrorCodes.h"
#include "Static.h"

#include "Emu/Memory/Memory.h"

// Most of the headers below rely on Memory.h
#include "lv2/lv2Fs.h"
#include "lv2/sys_cond.h"
#include "lv2/sys_event.h"
#include "lv2/sys_interrupt.h"
#include "lv2/sys_lwcond.h"
#include "lv2/sys_lwmutex.h"
#include "lv2/sys_memory.h"
#include "lv2/sys_mmapper.h"
#include "lv2/sys_ppu_thread.h"
#include "lv2/sys_process.h"
#include "lv2/sys_prx.h"
#include "lv2/sys_rsx.h"
#include "lv2/sys_rwlock.h"
#include "lv2/sys_semaphore.h"
#include "lv2/sys_spinlock.h"
#include "lv2/sys_spu.h"
#include "lv2/sys_time.h"
#include "lv2/sys_timer.h"
#include "lv2/sys_trace.h"
#include "lv2/sys_tty.h"
#include "lv2/sys_vm.h"

#include "Emu/Event.h"

#include "rpcs3/Ini.h"

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

	bool IsLogging()
	{
		return Ini.HLELogging.GetValue();
	}

	template<typename... Targs> void Notice(const u32 id, const char* fmt, Targs... args)
	{
		LOG_NOTICE(HLE, GetName() + fmt::Format("[%d]: ", id) + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Notice(const char* fmt, Targs... args)
	{
		LOG_NOTICE(HLE, GetName() + ": " + fmt::Format(fmt, args...));
	}

	template<typename... Targs> __forceinline void Log(const char* fmt, Targs... args)
	{
		if (IsLogging())
		{
			Notice(fmt, args...);
		}
	}

	template<typename... Targs> __forceinline void Log(const u32 id, const char* fmt, Targs... args)
	{
		if (IsLogging())
		{
			Notice(id, fmt, args...);
		}
	}

	template<typename... Targs> void Warning(const u32 id, const char* fmt, Targs... args)
	{
		LOG_WARNING(HLE, GetName() + fmt::Format("[%d] warning: ", id) + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Warning(const char* fmt, Targs... args)
	{
		LOG_WARNING(HLE, GetName() + " warning: " + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Error(const u32 id, const char* fmt, Targs... args)
	{
		LOG_ERROR(HLE, GetName() + fmt::Format("[%d] error: ", id) + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Error(const char* fmt, Targs... args)
	{
		LOG_ERROR(HLE, GetName() + " error: " + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Todo(const u32 id, const char* fmt, Targs... args)
	{
		LOG_ERROR(HLE, GetName() + fmt::Format("[%d] TODO: ", id) + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Todo(const char* fmt, Targs... args)
	{
		LOG_ERROR(HLE, GetName() + " TODO: " + fmt::Format(fmt, args...));
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

//cellGcm (used as lv2 syscall #1023)
extern int cellGcmCallback(u32 context_addr, u32 count);


#define UNIMPLEMENTED_FUNC(module) module->Todo("%s", __FUNCTION__)

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
	static std::string GetHLEFuncName(const u32 fid);
};

#define REG_SUB(module, group, name, ...) \
	static const u64 name ## _table[] = {__VA_ARGS__ , 0}; \
	module->AddFuncSub(group, name ## _table, #name, name)

#define REG_SUB_EMPTY(module, group, name,...) \
	static const u64 name ## _table[] = {0}; \
	module->AddFuncSub(group, name ## _table, #name, name)
