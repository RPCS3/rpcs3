#pragma once
#include <util/types.hpp>
#include <util/logs.hpp>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace utils
{
	namespace stack_trace
	{
		// Printing utilities

		template <typename T>
		concept Logger = requires (T& t, const std::string& msg)
		{
			{ t.print(msg) };
		};

		struct print_to_log
		{
			logs::channel& log;

		public:
			print_to_log(logs::channel& chan)
				: log(chan)
			{}

			void print(const std::string& s)
			{
				log.error("%s", s);
			}
		};
	}

#ifdef _WIN32
	std::vector<void*> get_backtrace(int max_depth = 255, PCONTEXT ctx = nullptr);
#else
	std::vector<void*> get_backtrace(int max_depth = 255);
#endif

	std::vector<std::string> get_backtrace_symbols(const std::vector<void*>& stack);

	FORCE_INLINE void print_trace(stack_trace::Logger auto& logger, int max_depth = 255)
	{
		const auto trace = get_backtrace(max_depth);
		const auto lines = get_backtrace_symbols(trace);

		for (const auto& line : lines)
		{
			logger.print(line);
		}
	}
}
