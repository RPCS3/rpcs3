#include "stdafx.h"
#include "SharedMutex.h"

void shared_mutex::impl_lock_shared(u32 old_value)
{
	// Throw if reader count breaks the "second" limit (it should be impossible)
	CHECK_ASSERTION((old_value & SM_READER_COUNT) != SM_READER_COUNT);

	std::unique_lock<std::mutex> lock(m_mutex);

	// Notify non-zero reader queue size
	m_ctrl |= SM_READER_QUEUE;

	// Compensate incorrectly increased reader count
	if ((--m_ctrl & SM_READER_COUNT) == 0 && m_wq_size)
	{
		// Notify current exclusive owner (condition passed)
		m_ocv.notify_one();
	}

	CHECK_ASSERTION(++m_rq_size);

	// Obtain the reader lock
	while (!atomic_op(m_ctrl, op_lock_shared))
	{
		m_rcv.wait(lock);
	}

	CHECK_ASSERTION(m_rq_size--);

	if (m_rq_size == 0)
	{
		m_ctrl &= ~SM_READER_QUEUE;
	}
}

void shared_mutex::impl_unlock_shared(u32 new_value)
{
	// Throw if reader count was zero
	CHECK_ASSERTION((new_value & SM_READER_COUNT) != SM_READER_COUNT);

	// Mutex cannot be unlocked before notification because m_ctrl has been changed outside
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_wq_size && (new_value & SM_READER_COUNT) == 0)
	{
		// Notify current exclusive owner that the latest reader is gone
		m_ocv.notify_one();
	}
	else if (m_rq_size)
	{
		m_rcv.notify_one();
	}
}

void shared_mutex::impl_lock_excl(u32 value)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	// Notify non-zero writer queue size
	m_ctrl |= SM_WRITER_QUEUE;

	CHECK_ASSERTION(++m_wq_size);

	// Obtain the writer lock
	while (!atomic_op(m_ctrl, op_lock_excl))
	{
		m_wcv.wait(lock);
	}

	// Wait for remaining readers
	while ((m_ctrl & SM_READER_COUNT) != 0)
	{
		m_ocv.wait(lock);
	}

	CHECK_ASSERTION(m_wq_size--);

	if (m_wq_size == 0)
	{
		m_ctrl &= ~SM_WRITER_QUEUE;
	}
}

void shared_mutex::impl_unlock_excl(u32 value)
{
	// Throw if was not locked exclusively
	CHECK_ASSERTION(value & SM_WRITER_LOCK);

	// Mutex cannot be unlocked before notification because m_ctrl has been changed outside
	std::lock_guard<std::mutex> lock(m_mutex);
	
	if (m_wq_size)
	{
		// Notify next exclusive owner
		m_wcv.notify_one();
	}
	else if (m_rq_size)
	{
		// Notify all readers
		m_rcv.notify_all();
	}
}
