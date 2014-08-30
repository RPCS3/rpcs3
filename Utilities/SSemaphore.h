#pragma once

class SSemaphore
{
	const u32 m_max;
	u32 m_count;
	u32 m_in_order;
	u32 m_out_order;
	std::mutex m_cv_mutex;
	std::mutex m_mutex;
	std::condition_variable m_cond;

public:
	SSemaphore(u32 value, u32 max = 1)
		: m_max(max > 0 ? max : 0xffffffff)
		, m_count(value > m_max ? m_max : value)
		, m_in_order(0)
		, m_out_order(0)
	{
	}

	SSemaphore()
		: m_max(0xffffffff)
		, m_count(0)
		, m_in_order(0)
		, m_out_order(0)
	{
	}

	~SSemaphore()
	{
	}

	void wait();

	bool try_wait();

	void post();

	bool post_and_wait();
};