#include "stdafx.h"
#include "Utilities/Semaphore.h"

bool semaphore_t::try_wait()
{
	// check m_value without interlocked op
	if (m_var.load().value == 0)
	{
		return false;
	}

	// try to decrement m_value atomically
	const auto old = m_var.atomic_op([](sync_var_t& var)
	{
		if (var.value)
		{
			var.value--;
		}
	});

	// recheck atomic result
	if (old.value == 0)
	{
		return false;
	}

	return true;
}

bool semaphore_t::try_post()
{
	// check m_value without interlocked op
	if (m_var.load().value >= max_value)
	{
		return false;
	}

	// try to increment m_value atomically
	const auto old = m_var.atomic_op([&](sync_var_t& var)
	{
		if (var.value < max_value)
		{
			var.value++;
		}
	});

	// recheck atomic result
	if (old.value >= max_value)
	{
		return false;
	}

	if (old.waiters)
	{
		// notify waiting thread
		std::lock_guard<std::mutex> lock(m_mutex);

		m_cv.notify_one();
	}

	return true;
}

void semaphore_t::wait()
{
	if (m_var.atomic_op([](sync_var_t& var) -> bool
	{
		if (var.value)
		{
			var.value--;

			return true;
		}
		else
		{
			//var.waiters++;

			return false;
		}
	}))
	{
		return;
	}

	std::unique_lock<std::mutex> lock(m_mutex);

	m_var.atomic_op([](sync_var_t& var)
	{
		var.waiters++;
	});

	while (!m_var.atomic_op([](sync_var_t& var) -> bool
	{
		if (var.value)
		{
			var.value--;
			var.waiters--;

			return true;
		}
		else
		{
			return false;
		}
	}))
	{
		m_cv.wait(lock);
	}
}

bool semaphore_t::post_and_wait()
{
	// TODO: merge these functions? Probably has a race condition.
	if (try_wait()) return false;

	try_post();
	wait();

	return true;
}
