#pragma once

class SSemaphore
{
	const u32 m_max;
	u32 m_count;
	std::mutex m_mutex, m_cv_mutex;
	std::condition_variable m_cond;

public:
	SSemaphore(u32 value, u32 max = 1)
		: m_max(max > 0 ? max : 0xffffffff)
		, m_count(value > m_max ? m_max : value)
	{
	}

	SSemaphore()
		: m_max(0xffffffff)
		, m_count(0)
	{
	}

	~SSemaphore()
	{
	}

	bool wait(u64 timeout = 0);

	bool try_wait();

	void post();

	bool post_and_wait();
};