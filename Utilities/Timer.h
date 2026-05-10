#pragma once

#include "util/types.hpp"

#include <chrono>

class Timer
{
private:
	bool m_stopped;
	steady_clock::time_point m_start;
	steady_clock::time_point m_end;

public:
	Timer() : m_stopped(false), m_start(steady_clock::now())
	{
	}

	void Start()
	{
		m_stopped = false;
		m_start = steady_clock::now();
	}

	void Stop()
	{
		m_stopped = true;
		m_end = steady_clock::now();
	}

	double GetElapsedTimeInSec() const
	{
		return static_cast<double>(GetElapsedTimeInMicroSec()) / 1000000.0;
	}

	double GetElapsedTimeInMilliSec() const
	{
		return static_cast<double>(GetElapsedTimeInMicroSec()) / 1000.0;
	}

	u64 GetElapsedTimeInMicroSec() const
	{
		const steady_clock::time_point now = m_stopped ? m_end : steady_clock::now();

		return std::chrono::duration_cast<std::chrono::microseconds>(now - m_start).count();
	}

	u64 GetElapsedTimeInNanoSec() const
	{
		const steady_clock::time_point now = m_stopped ? m_end : steady_clock::now();

		return std::chrono::duration_cast<std::chrono::nanoseconds>(now - m_start).count();
	}

	u64 GetMsSince(steady_clock::time_point timestamp) const
	{
		const steady_clock::time_point now = m_stopped ? m_end : steady_clock::now();

		return std::chrono::duration_cast<std::chrono::milliseconds>(now - timestamp).count();
	}
};
