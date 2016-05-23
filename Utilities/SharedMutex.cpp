#include "SharedMutex.h"

#include <mutex>
#include <condition_variable>

struct shared_mutex::internal
{
	std::mutex mutex;

	std::size_t rq_size{}; // Reader queue size (threads waiting on m_rcv)
	std::size_t wq_size{}; // Writer queue size (threads waiting on m_wcv and m_ocv)

	std::condition_variable rcv; // Reader queue
	std::condition_variable wcv; // Writer queue
	std::condition_variable ocv; // For current exclusive owner
};

void shared_mutex::lock_shared_hard()
{
	initialize_once();

	std::unique_lock<std::mutex> lock(m_data->mutex);

	// Validate
	if ((m_ctrl & SM_INVALID_BIT) != 0) throw std::runtime_error("shared_mutex::lock_shared(): Invalid bit");
	if ((m_ctrl & SM_READER_MASK) == 0) throw std::runtime_error("shared_mutex::lock_shared(): No readers");

	// Notify non-zero reader queue size
	m_ctrl |= SM_WAITERS_BIT, m_data->rq_size++;

	// Fix excess reader count
	if ((--m_ctrl & SM_READER_MASK) == 0 && m_data->wq_size)
	{
		// Notify exclusive owner
		m_data->ocv.notify_one();
	}

	// Obtain the reader lock
	while (true)
	{
		const auto ctrl = m_ctrl.load();

		// Check writers and reader limit
		if (m_data->wq_size || (ctrl & ~SM_WAITERS_BIT) >= SM_READER_MAX)
		{
			m_data->rcv.wait(lock);
			continue;
		}

		if (m_ctrl.compare_and_swap_test(ctrl, ctrl + 1))
		{
			break;
		}
	}

	if (!--m_data->rq_size && !m_data->wq_size)
	{
		m_ctrl &= ~SM_WAITERS_BIT;
	}
}

void shared_mutex::unlock_shared_notify()
{
	initialize_once();

	std::unique_lock<std::mutex> lock(m_data->mutex);

	if ((m_ctrl & SM_READER_MASK) == 0 && m_data->wq_size)
	{
		// Notify exclusive owner
		lock.unlock();
		m_data->ocv.notify_one();
	}
	else if (m_data->rq_size)
	{
		// Notify other readers
		lock.unlock();
		m_data->rcv.notify_one();
	}
}

void shared_mutex::lock_hard()
{
	initialize_once();

	std::unique_lock<std::mutex> lock(m_data->mutex);

	// Validate
	if ((m_ctrl & SM_INVALID_BIT) != 0) throw std::runtime_error("shared_mutex::lock(): Invalid bit");

	// Notify non-zero writer queue size
	m_ctrl |= SM_WAITERS_BIT, m_data->wq_size++;

	// Obtain the writer lock
	while (true)
	{
		const auto ctrl = m_ctrl.load();

		if (ctrl & SM_WRITER_LOCK)
		{
			m_data->wcv.wait(lock);
			continue;
		}

		if (m_ctrl.compare_and_swap_test(ctrl, ctrl | SM_WRITER_LOCK))
		{
			break;
		}
	}

	// Wait for remaining readers
	while ((m_ctrl & SM_READER_MASK) != 0)
	{
		m_data->ocv.wait(lock);
	}

	if (!--m_data->wq_size && !m_data->rq_size)
	{
		m_ctrl &= ~SM_WAITERS_BIT;
	}
}

void shared_mutex::unlock_notify()
{
	initialize_once();

	std::unique_lock<std::mutex> lock(m_data->mutex);

	if (m_data->wq_size)
	{
		// Notify next exclusive owner
		lock.unlock();
		m_data->wcv.notify_one();
	}
	else if (m_data->rq_size)
	{
		// Notify all readers
		lock.unlock();
		m_data->rcv.notify_all();
	}
}

void shared_mutex::lock_upgrade_hard()
{
	unlock_shared();
	lock();
}

void shared_mutex::lock_degrade_hard()
{
	initialize_once();

	std::unique_lock<std::mutex> lock(m_data->mutex);

	m_ctrl -= SM_WRITER_LOCK - 1;

	if (m_data->rq_size)
	{
		// Notify all readers
		lock.unlock();
		m_data->rcv.notify_all();
	}
	else if (m_data->wq_size)
	{
		// Notify next exclusive owner
		lock.unlock();
		m_data->wcv.notify_one();
	}
}

void shared_mutex::initialize_once()
{
	if (UNLIKELY(!m_data))
	{
		auto ptr = new shared_mutex::internal;

		if (!m_data.compare_and_swap_test(nullptr, ptr))
		{
			delete ptr;
		}
	}
}

shared_mutex::~shared_mutex()
{
	delete m_data;
}
