#pragma once
#include "ErrorCodes.h"
#include "LogBase.h"

//#define SYSCALLS_DEBUG

class SysCallBase : public LogBase
{
private:
	std::string m_module_name;

public:
	SysCallBase(const std::string& name)
		: m_module_name(name)
	{
	}

	virtual const std::string& GetName() const override
	{
		return m_module_name;
	}
};

extern bool dump_enable;

class PPUThread;

class SysCalls
{
public:
	static void DoSyscall(PPUThread& CPU, u64 code);
	static std::string GetHLEFuncName(const u32 fid);
};
