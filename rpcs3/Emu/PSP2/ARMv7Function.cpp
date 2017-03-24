#include "stdafx.h"
#include "ARMv7Module.h"

// Get function name by FNID
extern std::string arm_get_function_name(const std::string& module, u32 fnid)
{
	// Check registered functions
	if (const auto sm = arm_module_manager::get_module(module))
	{
		const auto found = sm->functions.find(fnid);

		if (found != sm->functions.end())
		{
			return found->second.name;
		}
	}

	return fmt::format("0x%08X", fnid);
}

// Get variable name by VNID
extern std::string arm_get_variable_name(const std::string& module, u32 vnid)
{
	// Check registered variables
	if (const auto sm = arm_module_manager::get_module(module))
	{
		const auto found = sm->variables.find(vnid);

		if (found != sm->variables.end())
		{
			return found->second.name;
		}
	}

	return fmt::format("0x%08X", vnid);
}

std::vector<arm_function_t>& arm_function_manager::access()
{
	static std::vector<arm_function_t> list
	{
		nullptr,
		[](ARMv7Thread& cpu) { cpu.state += cpu_flag::ret; },
	};

	return list;
}

u32 arm_function_manager::add_function(arm_function_t function)
{
	auto& list = access();

	list.push_back(function);

	return ::size32(list) - 1;
}

DECLARE(arm_function_manager::addr);
