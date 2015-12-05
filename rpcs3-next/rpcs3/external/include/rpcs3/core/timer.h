#pragma once

#include <chrono>

class Timer
{
private:
	bool m_stopped;
	std::chrono::high_resolution_clock::time_point m_start;
	std::chrono::high_resolution_clock::time_point m_end;

public:
	Timer() : m_stopped(false)
	{
	}

	void Start()
	{
		m_stopped = false;
		m_start = std::chrono::high_resolution_clock::now();
	}

	void Stop()
	{
		m_stopped = true;
		m_end = std::chrono::high_resolution_clock::now();
	}

	double GetElapsedTimeInSec() const
	{
		return double(GetElapsedTimeInMicroSec()) / 1000000.0;
	}

	double GetElapsedTimeInMilliSec() const
	{
		return double(GetElapsedTimeInMicroSec()) / 1000.0;
	}

	u64 GetElapsedTimeInMicroSec() const
	{
		std::chrono::high_resolution_clock::time_point now = m_stopped ? m_end : std::chrono::high_resolution_clock::now();

		return std::chrono::duration_cast<std::chrono::microseconds>(now - m_start).count();
	}

	u64 GetElapsedTimeInNanoSec() const
	{
		std::chrono::high_resolution_clock::time_point now = m_stopped ? m_end : std::chrono::high_resolution_clock::now();

		return std::chrono::duration_cast<std::chrono::nanoseconds>(now - m_start).count();
	}
};
