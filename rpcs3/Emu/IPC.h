#pragma once

#include <memory>
#include <unordered_map>

#include "Utilities/mutex.h"

// IPC manager for objects of type T and IPC keys of type K.
// External declaration of g_ipc is required.
template <typename T, typename K>
class ipc_manager final
{
	std::unordered_map<K, std::weak_ptr<T>> m_map;

	shared_mutex m_mutex;

	static ipc_manager g_ipc;

public:
	// Add new object if specified ipc_key is not used
	template <typename F>
	static bool add(const K& ipc_key, F&& provider, std::shared_ptr<T>* out = nullptr)
	{
		std::lock_guard lock(g_ipc.m_mutex);

		// Get object location
		std::weak_ptr<T>& wptr = g_ipc.m_map[ipc_key];

		std::shared_ptr<T> old;

		if ((out && !(old = wptr.lock())) || wptr.expired())
		{
			// Call a function which must return the object
			if (out)
			{
				*out = provider();
				wptr = *out;
			}
			else
			{
				wptr = provider();
			}

			return true;
		}

		if (out)
		{
			*out = std::move(old);
		}

		return false;
	}

	// Unregister specified ipc_key, may return true even if the object doesn't exist anymore
	static bool remove(const K& ipc_key)
	{
		std::lock_guard lock(g_ipc.m_mutex);

		return g_ipc.m_map.erase(ipc_key) != 0;
	}

	// Unregister specified ipc_key, return the object
	static std::shared_ptr<T> withdraw(const K& ipc_key)
	{
		std::lock_guard lock(g_ipc.m_mutex);

		const auto found = g_ipc.m_map.find(ipc_key);

		if (found != g_ipc.m_map.end())
		{
			auto ptr = found->second.lock();
			g_ipc.m_map.erase(found);
			return ptr;
		}

		return nullptr;
	}

	// Get object with specified ipc_key
	static std::shared_ptr<T> get(const K& ipc_key)
	{
		reader_lock lock(g_ipc.m_mutex);

		const auto found = g_ipc.m_map.find(ipc_key);

		if (found != g_ipc.m_map.end())
		{
			return found->second.lock();
		}

		return nullptr;
	}

	// Check whether the object actually exists
	static bool check(const K& ipc_key)
	{
		reader_lock lock(g_ipc.m_mutex);

		const auto found = g_ipc.m_map.find(ipc_key);

		return found != g_ipc.m_map.end() && !found->second.expired();
	}
};
