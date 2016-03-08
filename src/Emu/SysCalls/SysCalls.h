#pragma once

#include "ErrorCodes.h"

struct SysCallBase : public _log::channel
{
	SysCallBase(const std::string& name)
		: _log::channel(name, _log::level::notice)
	{
	}
};

void execute_syscall_by_index(class PPUThread& ppu, u64 code);
std::string get_ps3_function_name(u64 fid);
