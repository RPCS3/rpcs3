#include "stdafx.h"
#include "perf_meter.hpp"

void perf_stat_base::push(u64 ns[66]) noexcept
{
	for (u32 i = 0; i < 66; i++)
	{
		m_log[i] += ns[i];
	}
}

void perf_stat_base::print(const char* name) noexcept
{
	if (u64 num_total = m_log[0].load())
	{
		perf_log.notice("Perf stats for %s: total events: %u (total time %.4fs)", name, num_total, m_log[65].load() / 1000'000'000.);

		for (u32 i = 0; i < 13; i++)
		{
			if (u64 count = m_log[i + 1].load())
			{
				perf_log.notice("Perf stats for %s: events < %.3fµs: %u", name, std::pow(2., i) / 1000., count);
			}
		}

		for (u32 i = 14; i < 23; i++)
		{
			if (u64 count = m_log[i + 1].load()) [[unlikely]]
			{
				perf_log.notice("Perf stats for %s: events < %.3fms: %u", name, std::pow(2., i) / 1000'000., count);
			}
		}

		for (u32 i = 24; i < 33; i++)
		{
			if (u64 count = m_log[i + 1].load()) [[unlikely]]
			{
				perf_log.notice("Perf stats for %s: events < %.3fs: %u", name, std::pow(2., i) / 1000'000'000., count);
			}
		}

		for (u32 i = 34; i < 43; i++)
		{
			if (u64 count = m_log[i + 1].load()) [[unlikely]]
			{
				perf_log.notice("Perf stats for %s: events < %.0f SEC: %u", name, std::pow(2., i) / 1000'000'000., count);
			}
		}

		for (u32 i = 44; i < 63; i++)
		{
			if (u64 count = m_log[i + 1].load()) [[unlikely]]
			{
				perf_log.notice("Perf stats for %s: events < %.0f MIN: %u", name, std::pow(2., i) / 60'000'000'000., count);
			}
		}
	}
}
