#include "sleep_queue.h"

namespace common
{
	void sleep_queue_entry::add_entry()
	{
		m_queue.emplace_back(m_sleepable.shared_from_this());
	}

	void sleep_queue_entry::remove_entry()
	{
		for (auto it = m_queue.begin(); it != m_queue.end(); it++)
		{
			if (it->get() == &m_sleepable)
			{
				m_queue.erase(it);
				return;
			}
		}
	}

	bool sleep_queue_entry::find() const
	{
		for (auto it = m_queue.begin(); it != m_queue.end(); it++)
		{
			if (it->get() == &m_sleepable)
			{
				return true;
			}
		}

		return false;
	}

	sleep_queue_entry::sleep_queue_entry(sleepable& obj, sleep_queue& queue)
		: m_sleepable(obj)
		, m_queue(queue)
	{
		add_entry();
		obj.sleep();
	}

	sleep_queue_entry::sleep_queue_entry(sleepable& obj, sleep_queue& queue, const defer_sleep_t&)
		: m_sleepable(obj)
		, m_queue(queue)
	{
		obj.sleep();
	}

	sleep_queue_entry::~sleep_queue_entry()
	{
		remove_entry();
		m_sleepable.awake();
	}

	inline void sleep_queue_entry::enter()
	{
		add_entry();
	}

	inline void sleep_queue_entry::leave()
	{
		remove_entry();
	}

	inline sleep_queue_entry::operator bool() const
	{
		return find();
	}
}