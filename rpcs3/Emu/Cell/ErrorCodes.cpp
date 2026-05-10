#include "stdafx.h"
#include "ErrorCodes.h"
#include "PPUThread.h"
#include "SPUThread.h"

LOG_CHANNEL(sys_log, "SYS");

bool g_log_all_errors = false;

s32 error_code::error_report(s32 result, const logs::message* channel, const char* fmt, const fmt_type_info* sup, const u64* args)
{
	static thread_local std::string g_tls_error_str;
	static thread_local std::unordered_map<std::string, usz> g_tls_error_stats;

	if (!channel)
	{
		channel = &sys_log.error;
	}

	if (!sup && !args)
	{
		if (!fmt)
		{
			// Report and clean error state
			for (auto&& pair : g_tls_error_stats)
			{
				if (pair.second > 3)
				{
					channel->operator()("Stat: %s [x%u]", pair.first, pair.second);
				}
			}

			g_tls_error_stats.clear();
			return 0;
		}
	}

	ensure(fmt);

	const char* func = "Unknown function";

	if (auto ppu = get_current_cpu_thread<ppu_thread>())
	{
		if (auto current = ppu->current_function)
		{
			func = current;
		}
	}
	else if (auto spu = get_current_cpu_thread<spu_thread>())
	{
		if (auto current = spu->current_func; current && spu->start_time)
		{
			func = current;
		}
	}

	// Format log message (use preallocated buffer)
	g_tls_error_str.clear();

	fmt::append(g_tls_error_str, "'%s' failed with 0x%08x", func, result);

	// Add spacer between error and fmt if necessary
	if (fmt[0] != ' ')
		g_tls_error_str += " : ";

	fmt::raw_append(g_tls_error_str, fmt, sup, args);

	// Update stats and check log threshold

	if (g_log_all_errors) [[unlikely]]
	{
		if (!g_tls_error_stats.empty())
		{
			// Report and clean error state
			error_report(0, nullptr, nullptr, nullptr, nullptr);
		}

		channel->operator()("%s", g_tls_error_str);
	}
	else
	{
		const auto stat = ++g_tls_error_stats[g_tls_error_str];

		if (stat <= 3)
		{
			channel->operator()("%s [%u]", g_tls_error_str, stat);
		}
	}

	return result;
}
