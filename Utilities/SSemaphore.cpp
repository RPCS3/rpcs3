#include "stdafx.h"
#include "Utilities/SSemaphore.h"

bool SSemaphore::wait(u64 timeout)
{
	std::unique_lock<std::mutex> lock(m_cv_mutex);
	
	u64 counter = 0;
	while (true)
	{
		if (Emu.IsStopped())
		{
			return false;
		}
		if (timeout && counter >= timeout)
		{
			return false;
		}
		m_cond.wait_for(lock, std::chrono::milliseconds(1));
		counter++;

		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_count)
		{
			m_count--;
			return true;
		}
	}
}

bool SSemaphore::try_wait()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_count)
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

	if (m_count >= m_max)
	{
		return;
	}

	m_count++;
	m_cond.notify_one();
}

bool SSemaphore::post_and_wait()
{
	// TODO: ???
	if (try_wait()) return false;

	post();
	wait();

	return true;
}