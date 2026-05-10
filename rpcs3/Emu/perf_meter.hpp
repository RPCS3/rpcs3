#pragma once

#include "util/types.hpp"
#include "util/logs.hpp"
#include "util/tsc.hpp"
#include "system_config.h"
#include <array>
#include <cmath>

LOG_CHANNEL(perf_log, "PERF");

// TODO: constexpr with the help of bitcast
template <auto Name>
inline const auto perf_name = []
{
	constexpr auto short_name = Name;
	std::array<char, sizeof(Name) + 1> result{};
	std::memcpy(result.data(), &short_name, sizeof(Name));
	return result;
}();

class perf_stat_base
{
	atomic_t<u64> m_log[66]{};

protected:
	// Print accumulated values
	void print(const char* name) const noexcept;

	// Accumulate values from a thread
	void push(u64 ns[66]) noexcept;

	// Get end time; accumulate value to the TLS
	static void push(u64 data[66], u64 start_time, const char* name) noexcept;

	// Register TLS storage for stats
	static void add(u64 ns[66], const char* name) noexcept;

	// Unregister TLS storage and drain its data
	static void remove(u64 ns[66], const char* name) noexcept;

public:
	perf_stat_base() noexcept = default;

	perf_stat_base(const perf_stat_base&) = delete;

	perf_stat_base& operator =(const perf_stat_base&) = delete;

	~perf_stat_base() {}

	// Collect all data, report it, and clean
	static void report() noexcept;
};

// Object that prints event length stats at the end
template <auto ShortName>
class perf_stat final : public perf_stat_base
{
	static inline thread_local struct perf_stat_local
	{
		// Local non-atomic values for increments
		u64 m_log[66]{};

		perf_stat_local() noexcept
		{
			perf_stat_base::add(m_log, perf_name<ShortName>.data());
		}

		~perf_stat_local()
		{
			perf_stat_base::remove(m_log, perf_name<ShortName>.data());
		}

	} g_tls_perf_stat;

public:
	static FORCE_INLINE SAFE_BUFFERS(void) push([[maybe_unused]] u64 start_time) noexcept
	{
#if !defined(_WIN32) || defined(_MSC_VER) // Windows clang LTO doesn't seem to like this template
		perf_stat_base::push(g_tls_perf_stat.m_log, start_time, perf_name<ShortName>.data());
#endif
	}
};

// Object that prints event length at the end
template <auto ShortName, auto... SubEvents>
class perf_meter
{
	// Initialize array (possibly only 1 element) with timestamp
	u64 m_timestamps[1 + sizeof...(SubEvents)];

public:
	FORCE_INLINE SAFE_BUFFERS() perf_meter() noexcept
	{
		restart();
	}

	FORCE_INLINE SAFE_BUFFERS() perf_meter(int) noexcept
	{
		std::fill(std::begin(m_timestamps), std::end(m_timestamps), 0);
	}

	FORCE_INLINE SAFE_BUFFERS(operator bool) () const noexcept
	{
		return m_timestamps[0] != 0;
	}

	// Copy first timestamp
	template <auto SN, auto... S>
	FORCE_INLINE SAFE_BUFFERS() perf_meter(const perf_meter<SN, S...>& r) noexcept
	{
		m_timestamps[0] = r.get();
		std::memset(m_timestamps + 1, 0, sizeof(m_timestamps) - sizeof(u64));
	}

	template <auto SN, auto... S>
	SAFE_BUFFERS() perf_meter(perf_meter<SN, S...>&& r) noexcept
	{
		m_timestamps[0] = r.get();
		r.reset();
	}

	// Copy first timestamp
	template <auto SN, auto... S>
	SAFE_BUFFERS(perf_meter&) operator =(const perf_meter<SN, S...>& r) noexcept
	{
		m_timestamps[0] = r.get();
		return *this;
	}

	template <auto SN, auto... S>
	SAFE_BUFFERS(perf_meter&) operator =(perf_meter<SN, S...>& r) noexcept
	{
		m_timestamps[0] = r.get();
		r.reset();
		return *this;
	}

	// Push subevent data in array
	template <auto Event, usz Index = 0>
	SAFE_BUFFERS(void) push() noexcept
	{
		// TODO: should use more efficient search with type comparison, then value comparison, or pattern matching
		if constexpr (std::array<bool, sizeof...(SubEvents)>{(SubEvents == Event)...}[Index])
		{
			// Push actual timestamp into an array
			m_timestamps[Index + 1] = utils::get_tsc();
		}
		else if constexpr (Index < sizeof...(SubEvents))
		{
			// Proceed search recursively
			push<Event, Index + 1>();
		}
	}

	// Obtain initial timestamp
	u64 get() const noexcept
	{
		return m_timestamps[0];
	}

	// Disable this counter
	FORCE_INLINE SAFE_BUFFERS(void) reset() noexcept
	{
		m_timestamps[0] = 0;
	}

	// Re-initialize first timestamp
	FORCE_INLINE SAFE_BUFFERS(void) restart() noexcept
	{
		m_timestamps[0] = utils::get_tsc();
		std::memset(m_timestamps + 1, 0, sizeof(m_timestamps) - sizeof(u64));
	}

	SAFE_BUFFERS() ~perf_meter()
	{
		// Disabled counter
		if (!m_timestamps[0]) [[unlikely]]
		{
			return;
		}

		if (!g_cfg.core.perf_report) [[likely]]
		{
			return;
		}

		// Register perf stat in nanoseconds
		perf_stat<ShortName>::push(m_timestamps[0]);

		// TODO: handle push(), currently ignored
	}
};
