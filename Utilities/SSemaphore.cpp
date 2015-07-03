#include "stdafx.h"
#include "Utilities/SSemaphore.h"
#include "Emu/System.h"

void SSemaphore::wait()
{
	u32 order;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_count && m_out_order == m_in_order)
		{
			m_count--;
			return;
		}
		order = m_in_order++;
	}

	std::unique_lock<std::mutex> cv_lock(m_cv_mutex);
	
	while (true)
	{
		CHECK_EMU_STATUS;

		m_cond.wait_for(cv_lock, std::chrono::milliseconds(1));
		
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_count)
			{
				if (m_out_order == order)
				{
					m_count--;
					m_out_order++;
					return;
				}
				else
				{
					m_cond.notify_one();
				}
			}
		}
	}
}

bool SSemaphore::try_wait()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_count && m_in_order == m_out_order)
	{
		m_count--;
		return true;
	}
	else
	{
		return false;
	}
}

void SSemaphore::post()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_count < m_max)
	{
		m_count++;
	}
	else
	{
		return;
	}

	m_cond.notify_one();
}

bool SSemaphore::post_and_wait()
{
	// TODO: merge these functions? Probably has a race condition.
	if (try_wait()) return false;

	post();
	wait();

	return true;
}
