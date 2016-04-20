#pragma once

#include <memory>
#include <unordered_map>

#include "Utilities/SharedMutex.h"

// IPC manager for lv2 objects of type T and 64-bit IPC keys.
// External declaration of g_ipc is required.
template<typename T>
class ipc_manager final
{
	std::unordered_map<u64, std::weak_ptr<T>> m_map;

	shared_mutex m_mutex;

	static ipc_manager g_ipc;

public:
	// Add new object if specified ipc_key is not used
	template<typename F>
	static auto add(u64 ipc_key, F&& provider) -> decltype(static_cast<std::shared_ptr<T>>(provider()))
	{
		writer_lock lock(g_ipc.m_mutex);

		// Get object location
		std::weak_ptr<T>& wptr = g_ipc.m_map[ipc_key];

		if (wptr.expired())
		{
			// Call a function which must return the object
			std::shared_ptr<T> result = provider();
			wptr = result;
			return result;
		}

		return{};
	}

	// Get existing object with specified ipc_key
	static std::shared_ptr<T> get(u64 ipc_key)
	{
		reader_lock lock(g_ipc.m_mutex);

		const auto found = g_ipc.m_map.find(ipc_key);

		if (found != g_ipc.m_map.end())
		{
			return found->second.lock();
		}

		return{};
	}
};
