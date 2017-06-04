#pragma once
#include <deque>
#include <memory>

namespace common
{
	struct sleepable : std::enable_shared_from_this<sleepable>
	{
		virtual void sleep() = 0;
		virtual void awake() = 0;
	};

	using sleep_queue = std::deque<std::shared_ptr<sleepable>>;

	static struct defer_sleep_t {} const defer_sleep{};

	// automatic object handling a thread entry in the sleep queue
	class sleep_queue_entry final
	{
		sleepable& m_sleepable;
		sleep_queue& m_queue;

		void add_entry();
		void remove_entry();
		bool find() const;

	public:
		// add specified thread to the sleep queue
		sleep_queue_entry(sleepable& obj, sleep_queue& queue);

		// don't add specified thread to the sleep queue
		sleep_queue_entry(sleepable& obj, sleep_queue& queue, const defer_sleep_t&);

		// removes specified thread from the sleep queue if added
		~sleep_queue_entry();

		// add thread to the sleep queue
		void enter();

		// remove thread from the sleep queue
		void leave();

		// check whether the thread exists in the sleep queue
		explicit operator bool() const;
	};
}