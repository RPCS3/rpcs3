﻿#include "stdafx.h"
#include "perf_meter.hpp"

#include <map>
#include <mutex>

void perf_stat_base::push(u64 ns[66]) noexcept
{
	if (!ns[0])
	{
		return;
	}

	for (u32 i = 0; i < 66; i++)
	{
		m_log[i] += atomic_storage<u64>::exchange(ns[i], 0);
	}
}

void perf_stat_base::print(const char* name) noexcept
{
	if (u64 num_total = m_log[0].load())
	{
		perf_log.notice(u8"Perf stats for %s: total events: %u (total time %.4fs, avg %.4fµs)", name, num_total, m_log[65].load() / 1000'000'000., m_log[65].load() / 1000. / num_total);

		for (u32 i = 0; i < 13; i++)
		{
			if (u64 count = m_log[i + 1].load())
			{
				perf_log.notice(u8"Perf stats for %s: events < %.3fµs: %u", name, std::pow(2., i) / 1000., count);
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

static shared_mutex s_perf_mutex;

static std::map<std::string, perf_stat_base> s_perf_acc;

static std::multimap<std::string, u64*> s_perf_sources;

void perf_stat_base::add(u64 ns[66], const char* name) noexcept
{
	std::lock_guard lock(s_perf_mutex);

	s_perf_sources.emplace(name, ns);
	s_perf_acc[name];
}

void perf_stat_base::remove(u64 ns[66], const char* name) noexcept
{
	std::lock_guard lock(s_perf_mutex);

	const auto found = s_perf_sources.equal_range(name);

	for (auto it = found.first; it != found.second; it++)
	{
		if (it->second == ns)
		{
			s_perf_acc[name].push(ns);
			s_perf_sources.erase(it);
			break;
		}
	}
}

void perf_stat_base::report() noexcept
{
	std::lock_guard lock(s_perf_mutex);

	perf_log.notice("Performance report begin (%u src, %u acc):", s_perf_sources.size(), s_perf_acc.size());

	for (auto& [name, ns] : s_perf_sources)
	{
		s_perf_acc[name].push(ns);
	}

	for (auto& [name, data] : s_perf_acc)
	{
		data.print(name.c_str());
	}

	s_perf_acc.clear();

	perf_log.notice("Performance report end.");
}
