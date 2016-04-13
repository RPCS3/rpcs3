#pragma once

#include "Utilities/SharedMutex.h"
#include "Utilities/SleepQueue.h"

namespace vm { using namespace ps3; }

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

// IPC manager collection for lv2 objects of type T
template<typename T>
class ipc_manager final
{
	mutable shared_mutex m_mutex;
	std::unordered_map<u64, std::weak_ptr<T>> m_map;

public:
	// Add new object if specified ipc_key is not used
	template<typename F>
	auto add(u64 ipc_key, F&& provider) -> decltype(static_cast<std::shared_ptr<T>>(provider()))
	{
		std::lock_guard<shared_mutex> lock(m_mutex);
		
		// Get object location
		std::weak_ptr<T>& wptr = m_map[ipc_key];

		if (wptr.expired())
		{
			// Call a function which must return the object
			std::shared_ptr<T>&& result = provider();
			wptr = result;
			return result;
		}

		return{};
	}

	// Get existing object with specified ipc_key
	std::shared_ptr<T> get(u64 ipc_key) const
	{
		reader_lock lock(m_mutex);

		const auto found = m_map.find(ipc_key);

		if (found != m_map.end())
		{
			return found->second.lock();
		}

		return{};
	}
};

// Simple class for global mutex to pass unique_lock and check it
struct lv2_lock_t
{
	using type = std::unique_lock<std::mutex>;

	type& ref;

	lv2_lock_t(type& lv2_lock)
		: ref(lv2_lock)
	{
		Expects(ref.owns_lock());
		Expects(ref.mutex() == &mutex);
	}

	operator type&() const
	{
		return ref;
	}

	static type::mutex_type mutex;
};

#define LV2_LOCK lv2_lock_t::type lv2_lock(lv2_lock_t::mutex)
