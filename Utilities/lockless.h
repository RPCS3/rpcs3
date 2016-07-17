#pragma once

#include "types.h"
#include "Atomic.h"
#include "Platform.h"

//! Simple sizeless array base for concurrent access. Cannot shrink, only growths automatically.
//! There is no way to know the current size. The smaller index is, the faster it's accessed.
//! 
//! T is the type of elements. Currently, default constructor of T shall be constexpr.
//! N is initial element count, available without any memory allocation and only stored contiguously.
template<typename T, std::size_t N>
class lf_array
{
	// Data (default-initialized)
	T m_data[N]{};

	// Next array block
	atomic_t<lf_array*> m_next{};

public:
	constexpr lf_array() = default;

	~lf_array()
	{
		for (auto ptr = m_next.raw(); UNLIKELY(ptr);)
		{
			delete std::exchange(ptr, std::exchange(ptr->m_next.raw(), nullptr));
		}
	}

	T& operator [](std::size_t index)
	{
		if (LIKELY(index < N))
		{
			return m_data[index];
		}
		else if (UNLIKELY(!m_next))
		{
			// Create new array block. It's not a full-fledged once-synchronization, unlikely needed.
			for (auto _new = new lf_array, ptr = this; UNLIKELY(ptr);)
			{
				// Install the pointer. If failed, go deeper.
				ptr = ptr->m_next.compare_and_swap(nullptr, _new);
			}
		}

		// Access recursively
		return (*m_next)[index - N];
	}
};

//! Simple lock-free FIFO queue base. Based on lf_array<T, N> itself. Currently uses 32-bit counters.
//! There is no "push_end" or "pop_begin" provided, the queue element must signal its state on its own.
template<typename T, std::size_t N>
class lf_fifo : public lf_array<T, N>
{
	struct alignas(8) ctrl_t
	{
		u32 push;
		u32 pop;
	};

	atomic_t<ctrl_t> m_ctrl{};

public:
	constexpr lf_fifo() = default;

	// Get current "push" position
	u32 size()
	{
		return reinterpret_cast<atomic_t<u32>&>(m_ctrl).load(); // Hack
	}
	
	// Acquire the place for one or more elements.
	u32 push_begin(u32 count = 1)
	{
		return reinterpret_cast<atomic_t<u32>&>(m_ctrl).fetch_add(count); // Hack
	}

	// Get current "pop" position
	u32 peek()
	{
		return m_ctrl.load().pop;
	}

	// Acknowledge processed element, return number of the next one.
	// Perform clear if possible, zero is returned in this case.
	u32 pop_end(u32 count = 1)
	{
		return m_ctrl.atomic_op([&](ctrl_t& ctrl)
		{
			ctrl.pop += count;

			if (ctrl.pop == ctrl.push)
			{
				// Clean if possible
				ctrl.push = 0;
				ctrl.pop = 0;
			}

			return ctrl.pop;
		});
	}
};

//! Simple lock-free map. Based on lf_array<>. All elements are accessible, implicitly initialized.
template<typename K, typename T, typename Hash = value_hash<K>, std::size_t Size = 256>
class lf_hashmap
{
	struct pair_t
	{
		// Default-constructed key means "no key"
		atomic_t<K> key{};
		T value{};
	};

	//
	lf_array<pair_t, Size> m_data{};

	// Value for default-constructed key
	T m_default_key_data{};

public:
	constexpr lf_hashmap() = default;

	// Access element (added implicitly)
	T& operator [](const K& key)
	{
		if (UNLIKELY(key == K{}))
		{
			return m_default_key_data;
		}
		
		// Calculate hash and array position
		for (std::size_t pos = Hash{}(key) % Size;; pos += Size)
		{
			// Access the array
			auto& pair = m_data[pos];

			// Check the key value (optimistic)
			if (LIKELY(pair.key == key) || pair.key.compare_and_swap_test(K{}, key))
			{
				return pair.value;
			}
		}
	}
};
