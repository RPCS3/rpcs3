#pragma once

class CPUThread;

using sleep_queue_t = std::deque<std::shared_ptr<CPUThread>>;

static struct defer_sleep_t {} const defer_sleep{};

// automatic object handling a thread entry in the sleep queue
class sleep_queue_entry_t final
{
	CPUThread& m_thread;
	sleep_queue_t& m_queue;

	void add_entry();
	void remove_entry();
	bool find() const;

public:
	// add specified thread to the sleep queue
	sleep_queue_entry_t(CPUThread& cpu, sleep_queue_t& queue);

	// don't add specified thread to the sleep queue
	sleep_queue_entry_t(CPUThread& cpu, sleep_queue_t& queue, const defer_sleep_t&);

	// removes specified thread from the sleep queue if added
	~sleep_queue_entry_t() noexcept(false);

	// add thread to the sleep queue
	inline void enter()
	{
		add_entry();
	}

	// remove thread from the sleep queue
	inline void leave()
	{
		remove_entry();
	}

	// check whether the thread exists in the sleep queue
	inline explicit operator bool() const
	{
		return find();
	}
};
