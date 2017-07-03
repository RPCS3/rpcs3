#pragma once

#include "Utilities/mutex.h"
#include "Utilities/sema.h"
#include "Utilities/cond.h"

#include "Emu/Memory/vm.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/ErrorCodes.h"

#include <deque>

// attr_protocol (waiting scheduling policy)
enum
{
	// First In, First Out
	SYS_SYNC_FIFO = 1,
	// Priority Order
	SYS_SYNC_PRIORITY = 2,
	// Basic Priority Inheritance Protocol (probably not implemented)
	SYS_SYNC_PRIORITY_INHERIT = 3,
	// Not selected while unlocking
	SYS_SYNC_RETRY = 4,
	//
	SYS_SYNC_ATTR_PROTOCOL_MASK = 0xF,
};

// attr_recursive (recursive locks policy)
enum
{
	// Recursive locks are allowed
	SYS_SYNC_RECURSIVE = 0x10,
	// Recursive locks are NOT allowed
	SYS_SYNC_NOT_RECURSIVE = 0x20,
	//
	SYS_SYNC_ATTR_RECURSIVE_MASK = 0xF0, //???
};

// attr_pshared
enum
{
	SYS_SYNC_NOT_PROCESS_SHARED = 0x200,
};

// attr_adaptive
enum
{
	SYS_SYNC_ADAPTIVE     = 0x1000,
	SYS_SYNC_NOT_ADAPTIVE = 0x2000,
};

// Base class for some kernel objects (shared set of 8192 objects).
struct lv2_obj
{
	using id_type = lv2_obj;

	static const u32 id_step = 0x100;
	static const u32 id_count = 8192;

	// Find and remove the object from the container (deque or vector)
	template <typename T, typename E>
	static bool unqueue(std::deque<T*>& queue, const E& object)
	{
		for (auto found = queue.cbegin(), end = queue.cend(); found != end; found++)
		{
			if (*found == object)
			{
				queue.erase(found);
				return true;
			}
		}

		return false;
	}

	template <typename E, typename T>
	static T* schedule(std::deque<T*>& queue, u32 protocol)
	{
		if (queue.empty())
		{
			return nullptr;
		}

		if (protocol == SYS_SYNC_FIFO)
		{
			const auto res = queue.front();
			queue.pop_front();
			return res;
		}

		u32 prio = -1;
		auto it = queue.cbegin();

		for (auto found = it, end = queue.cend(); found != end; found++)
		{
			const u32 _prio = static_cast<E*>(*found)->prio;

			if (_prio < prio)
			{
				it = found;
				prio = _prio;
			}
		}

		const auto res = *it;
		queue.erase(it);
		return res;
	}

	// Remove the current thread from the scheduling queue, register timeout
	static void sleep_timeout(named_thread&, u64 timeout);

	static void sleep(cpu_thread& thread, u64 timeout = 0)
	{
		vm::temporary_unlock(thread);
		sleep_timeout(thread, timeout);
	}

	// Schedule the thread
	static void awake(cpu_thread&, u32 prio);

	static void awake(cpu_thread& thread)
	{
		awake(thread, -1);
	}

	static void cleanup();

private:
	// Scheduler mutex
	static semaphore<> g_mutex;

	// Scheduler queue for active PPU threads
	static std::deque<class ppu_thread*> g_ppu;

	// Waiting for the response from
	static std::deque<class cpu_thread*> g_pending;

	// Scheduler queue for timeouts (wait until -> thread)
	static std::deque<std::pair<u64, named_thread*>> g_waiting;

	static void schedule_all();
};
