#pragma once
#include "ErrorCodes.h"
#include "LogBase.h"

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

void execute_syscall_by_index(class PPUThread& ppu, u64 code);
std::string get_ps3_function_name(u64 fid);
