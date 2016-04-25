#include "Utilities/Semaphore.h"

#include <mutex>
#include <condition_variable>

struct benaphore::internal
{
	std::mutex mutex;

	std::size_t acq_order{};
	std::size_t rel_order{};

	std::condition_variable cond;
};

void benaphore::wait_hard()
{
	initialize_once();

	std::unique_lock<std::mutex> lock(m_data->mutex);

	// Notify non-zero waiter queue size
	if (m_value.exchange(-1) == 1)
	{
		// Return immediately (acquired)
		m_value = 0;
		return;
	}

	// Remember the order
	const std::size_t order = ++m_data->acq_order;

	// Wait for the appropriate rel_order (TODO)
	while (m_data->rel_order < order)
	{
		m_data->cond.wait(lock);
	}

	if (order == m_data->acq_order && m_data->acq_order == m_data->rel_order)
	{
		// Cleaup
		m_data->acq_order = 0;
		m_data->rel_order = 0;
		m_value.compare_and_swap(-1, 0);
	}
}

void benaphore::post_hard()
{
	initialize_once();

	std::unique_lock<std::mutex> lock(m_data->mutex);

	if (m_value.compare_and_swap(0, 1) != -1)
	{
		// Do nothing (released)
		return;
	}

	if (m_data->acq_order == m_data->rel_order)
	{
		m_value = 1;
		return;
	}

	// Awake one thread
	m_data->rel_order += 1;
		
	// Unlock and notify
	lock.unlock();
	m_data->cond.notify_one();
}

void benaphore::initialize_once()
{
	if (UNLIKELY(!m_data))
	{
		auto ptr = new benaphore::internal;

		if (!m_data.compare_and_swap_test(nullptr, ptr))
		{
			delete ptr;
		}
	}
}

benaphore::~benaphore()
{
	delete m_data;
}
