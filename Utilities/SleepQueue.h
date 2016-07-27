#pragma once

#include <deque>

// Tag used in sleep_entry<> constructor
static struct defer_sleep_tag {} constexpr defer_sleep{};

// Define sleep queue as std::deque with T* pointers, T - thread type
template<typename T> using sleep_queue = std::deque<T*>;

// Automatic object handling a thread pointer (T*) in the sleep queue
// Sleep is called in the constructor (if not null)
// Awake is called in the destructor (if not null)
// Sleep queue is actually std::deque with pointers, be careful about the lifetime
template<typename T, void(T::*Sleep)() = nullptr, void(T::*Awake)() = nullptr>
class sleep_entry final
{
	sleep_queue<T>& m_queue;
	T& m_thread;

public:
	// Constructor; enter() not called
	sleep_entry(sleep_queue<T>& queue, T& entry, const defer_sleep_tag&)
		: m_queue(queue)
		, m_thread(entry)
	{
		if (Sleep) (m_thread.*Sleep)();
	}

	// Constructor; calls enter()
	sleep_entry(sleep_queue<T>& queue, T& entry)
		: sleep_entry(queue, entry, defer_sleep)
	{
		enter();
	}

	// Destructor; calls leave()
	~sleep_entry()
	{
		leave();
		if (Awake) (m_thread.*Awake)();
	}

	// Add thread to the sleep queue
	void enter()
	{
		for (auto t : m_queue)
		{
			if (t == &m_thread)
			{
				// Already exists, is it an error?
				return;
			}
		}
		
		m_queue.emplace_back(&m_thread);
	}

	// Remove thread from the sleep queue
	void leave()
	{
		for (auto it = m_queue.begin(), end = m_queue.end(); it != end; it++)
		{
			if (*it == &m_thread)
			{
				m_queue.erase(it);
				return;
			}
		}
	}

	// Check whether the thread exists in the sleep queue
	explicit operator bool() const
	{
		for (auto it = m_queue.begin(), end = m_queue.end(); it != end; it++)
		{
			if (*it == &m_thread)
			{
				return true;
			}
		}

		return false;
	}
};
