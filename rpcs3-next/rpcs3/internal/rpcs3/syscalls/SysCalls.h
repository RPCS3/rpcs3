#pragma once

#include "error_codes.h"
#include "log_base.h"
#include <rpcs3/core/basic_types.h>
#include <string>

namespace rpcs3::syscalls
{
	class syscall : public log_base
	{
	private:
		std::string m_module_name;

	public:
		syscall(const std::string& name)
			: m_module_name(name)
		{
		}

		virtual const std::string& name() const override
		{
			return m_module_name;
		}
	};

	void execute_by_index(class PPUThread& ppu, u64 code);
	std::string get_ps3_function_name(u64 fid);
}