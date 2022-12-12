#pragma once
#include <util/types.hpp>
#include <util/logs.hpp>

namespace utils
{
	std::vector<void*> get_backtrace(int max_depth = 255);
	std::vector<std::string> get_backtrace_symbols(const std::vector<void*>& stack);

	FORCE_INLINE void print_trace(logs::channel& logger, int max_depth = 255)
	{
		const auto trace = get_backtrace(max_depth);
		const auto lines = get_backtrace_symbols(trace);

		for (const auto& line : lines)
		{
			logger.error("%s", line);
		}
	}
}
