#pragma once

#include <unordered_map>

#include "Utilities/mutex.h"

#include "util/shared_ptr.hpp"

// IPC manager for objects of type T and IPC keys of type K.
template <typename T, typename K>
class ipc_manager final
{
	std::unordered_map<K, shared_ptr<T>> m_map;

	mutable shared_mutex m_mutex;

public:
	// Add new object if specified ipc_key is not used
	// .first: added new object?, .second: what's at m_map[key] after this function if (peek_ptr || added new object) is true
	template <typename F>
	std::pair<bool, shared_ptr<T>> add(const K& ipc_key, F&& provider, bool peek_ptr = true)
	{
		std::lock_guard lock(m_mutex);

		// Get object location
		shared_ptr<T>& ptr = m_map[ipc_key];
		const bool existed = ptr.operator bool();

		if (!existed)
		{
			// Call a function which must return the object
			ptr = provider();
		}

		const bool added = !existed && ptr;
		return {added, (peek_ptr || added) ? ptr : null_ptr};
	}

	// Unregister specified ipc_key, may return true even if the object doesn't exist anymore
	bool remove(const K& ipc_key)
	{
		std::lock_guard lock(m_mutex);

		return m_map.erase(ipc_key) != 0;
	}

	// Get object with specified ipc_key
	shared_ptr<T> get(const K& ipc_key) const
	{
		reader_lock lock(m_mutex);

		const auto found = m_map.find(ipc_key);

		if (found != m_map.end())
		{
			return found->second;
		}

		return {};
	}

	// Check whether the object actually exists
	bool check(const K& ipc_key) const
	{
		reader_lock lock(m_mutex);

		const auto found = m_map.find(ipc_key);

		return found != m_map.end();
	}
};
