#pragma once

#include "types.h"
#include "Atomic.h"

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
	u32 size() const
	{
		return reinterpret_cast<const atomic_t<u32>&>(m_ctrl).load(); // Hack
	}
	
	// Acquire the place for one or more elements.
	u32 push_begin(u32 count = 1)
	{
		return reinterpret_cast<atomic_t<u32>&>(m_ctrl).fetch_add(count); // Hack
	}

	// Get current "pop" position
	u32 peek() const
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

// Fixed-size single-producer single-consumer queue
template <typename T, std::uint32_t N>
class lf_spsc
{
	// If N is a power of 2, m_push/m_pop can safely overflow and the algorithm is simplified
	static_assert(N && (1u << 31) % N == 0, "lf_spsc<> error: size must be power of 2");

protected:
	volatile std::uint32_t m_push{0};
	volatile std::uint32_t m_pop{0};
	T m_data[N]{};

public:
	constexpr lf_spsc() = default;

	// Try to push (producer only)
	template <typename T2>
	bool try_push(T2&& data)
	{
		const std::uint32_t pos = m_push;

		if (pos - m_pop >= N)
		{
			return false;
		}

		_mm_lfence();
		m_data[pos % N] = std::forward<T2>(data);
		_mm_sfence();
		m_push = pos + 1;
		return true;
	}

	// Try to get push pointer (producer only)
	operator T*()
	{
		const std::uint32_t pos = m_push;

		if (pos - m_pop >= N)
		{
			return nullptr;
		}

		_mm_lfence();
		return m_data + (pos % N);
	}

	// Increment push counter (producer only)
	void end_push()
	{
		const std::uint32_t pos = m_push;

		if (pos - m_pop < N)
		{
			_mm_sfence();
			m_push = pos + 1;
		}
	}

	// Unsafe access
	T& get_push(std::size_t i)
	{
		_mm_lfence();
		return m_data[(m_push + i) % N];
	}

	// Try to pop (consumer only)
	template <typename T2>
	bool try_pop(T2& out)
	{
		const std::uint32_t pos = m_pop;

		if (m_push - pos <= 0)
		{
			return false;
		}

		_mm_lfence();
		out = std::move(m_data[pos % N]);
		_mm_sfence();
		m_pop = pos + 1;
		return true;
	}

	// Increment pop counter (consumer only)
	void end_pop()
	{
		const std::uint32_t pos = m_pop;

		if (m_push - pos > 0)
		{
			_mm_sfence();
			m_pop = pos + 1;
		}
	}

	// Get size (consumer only)
	std::uint32_t size() const
	{
		return m_push - m_pop;
	}

	// Direct access (consumer only)
	T& operator [](std::size_t i)
	{
		_mm_lfence();
		return m_data[(m_pop + i) % N];
	}
};

// Fixed-size multi-producer single-consumer queue
template <typename T, std::uint32_t N>
class lf_mpsc : lf_spsc<T, N>
{
protected:
	using lf_spsc<T, N>::m_push;
	using lf_spsc<T, N>::m_pop;
	using lf_spsc<T, N>::m_data;

	enum : std::uint64_t
	{
		c_ack = 1ull << 0,
		c_rel = 1ull << 32,
	};

	atomic_t<std::uint64_t> m_lock{0};

	void release(std::uint64_t value)
	{
		// Push all pending elements at once when possible
		if (value && value % c_rel == value / c_rel)
		{
			_mm_sfence();
			m_push += value % c_rel;
			m_lock.compare_and_swap_test(value, 0);
		}
	}

public:
	constexpr lf_mpsc() = default;

	// Try to get push pointer
	operator T*()
	{
		const std::uint64_t old = m_lock.fetch_add(c_ack);
		const std::uint32_t pos = m_push;

		if (old % N >= N || pos - m_pop >= N - (old % N))
		{
			release(m_lock.sub_fetch(c_ack));
			return nullptr;
		}

		return m_data + ((pos + old) % N);
	}

	// Increment push counter (producer only)
	void end_push()
	{
		release(m_lock.add_fetch(c_rel));
	}

	// Try to push
	template <typename T2>
	bool try_push(T2&& data)
	{
		if (T* ptr = *this)
		{
			*ptr = std::forward<T2>(data);
			end_push();
			return true;
		}

		return false;
	}

	// Enable consumer methods
	using lf_spsc<T, N>::try_pop;
	using lf_spsc<T, N>::end_pop;
	using lf_spsc<T, N>::size;
	using lf_spsc<T, N>::operator [];
};
