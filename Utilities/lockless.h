#pragma once

#include "types.h"
#include "Atomic.h"

//! Simple sizeless array base for concurrent access. Cannot shrink, only growths automatically.
//! There is no way to know the current size. The smaller index is, the faster it's accessed.
//!
//! T is the type of elements. Currently, default constructor of T shall be constexpr.
//! N is initial element count, available without any memory allocation and only stored contiguously.
template <typename T, std::size_t N>
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

// Helper type, linked list element
template <typename T>
class lf_queue_item final
{
	lf_queue_item* m_link = nullptr;

	T m_data;

	template <typename U>
	friend class lf_queue_iterator;

	template <typename U>
	friend class lf_queue_slice;

	template <typename U>
	friend class lf_queue;

	constexpr lf_queue_item() = default;

	template <typename... Args>
	constexpr lf_queue_item(lf_queue_item* link, Args&&... args)
	    : m_link(link)
	    , m_data(std::forward<Args>(args)...)
	{
	}

public:
	lf_queue_item(const lf_queue_item&) = delete;

	lf_queue_item& operator=(const lf_queue_item&) = delete;

	~lf_queue_item()
	{
		for (lf_queue_item* ptr = m_link; ptr;)
		{
			delete std::exchange(ptr, std::exchange(ptr->m_link, nullptr));
		}
	}
};

// Forward iterator: non-owning pointer to the list element in lf_queue_slice<>
template <typename T>
class lf_queue_iterator
{
	lf_queue_item<T>* m_ptr = nullptr;

	template <typename U>
	friend class lf_queue_slice;

public:
	constexpr lf_queue_iterator() = default;

	bool operator ==(const lf_queue_iterator& rhs) const
	{
		return m_ptr == rhs.m_ptr;
	}

	bool operator !=(const lf_queue_iterator& rhs) const
	{
		return m_ptr != rhs.m_ptr;
	}

	T& operator *() const
	{
		return m_ptr->m_data;
	}

	T* operator ->() const
	{
		return &m_ptr->m_data;
	}

	lf_queue_iterator& operator ++()
	{
		m_ptr = m_ptr->m_link;
		return *this;
	}

	lf_queue_iterator operator ++(int)
	{
		lf_queue_iterator result;
		result.m_ptr = m_ptr;
		m_ptr = m_ptr->m_link;
		return result;
	}
};

// Owning pointer to the linked list taken from the lf_queue<>
template <typename T>
class lf_queue_slice
{
	lf_queue_item<T>* m_head = nullptr;

	template <typename U>
	friend class lf_queue;

public:
	constexpr lf_queue_slice() = default;

	lf_queue_slice(const lf_queue_slice&) = delete;

	lf_queue_slice(lf_queue_slice&& r) noexcept
		: m_head(r.m_head)
	{
		r.m_head = nullptr;
	}

	lf_queue_slice& operator =(const lf_queue_slice&) = delete;

	lf_queue_slice& operator =(lf_queue_slice&& r) noexcept
	{
		if (this != &r)
		{
			delete m_head;
			m_head = r.m_head;
			r.m_head = nullptr;
		}

		return *this;
	}

	~lf_queue_slice()
	{
		delete m_head;
	}

	T& operator *() const
	{
		return m_head->m_data;
	}

	T* operator ->() const
	{
		return &m_head->m_data;
	}

	explicit operator bool() const
	{
		return m_head != nullptr;
	}

	T* get() const
	{
		return m_head ? &m_head->m_data : nullptr;
	}

	lf_queue_iterator<T> begin() const
	{
		lf_queue_iterator<T> result;
		result.m_ptr = m_head;
		return result;
	}

	lf_queue_iterator<T> end() const
	{
		return {};
	}

	lf_queue_slice& pop_front()
	{
		delete std::exchange(m_head, std::exchange(m_head->m_link, nullptr));
		return *this;
	}
};

class lf_queue_base
{
protected:
	atomic_t<std::uintptr_t> m_head = 0;

	void imp_notify();

public:
	// Wait for new elements pushed, no other thread shall call wait() or pop_all() simultaneously
	bool wait(u64 usec_timeout = -1);
};

// Linked list-based multi-producer queue (the consumer drains the whole queue at once)
template <typename T>
class lf_queue : public lf_queue_base
{
	using lf_queue_base::m_head;

	// Extract all elements and reverse element order (FILO to FIFO)
	lf_queue_item<T>* reverse() noexcept
	{
		if (auto* head = m_head.load() ? reinterpret_cast<lf_queue_item<T>*>(m_head.exchange(0)) : nullptr)
		{
			if (auto* prev = head->m_link)
			{
				head->m_link = nullptr;

				do
				{
					auto* pprev  = prev->m_link;
					prev->m_link = head;
					head         = std::exchange(prev, pprev);
				}
				while (prev);
			}

			return head;
		}

		return nullptr;
	}

public:
	constexpr lf_queue() = default;

	~lf_queue()
	{
		delete reinterpret_cast<lf_queue_item<T>*>(m_head.load());
	}

	template <typename... Args>
	void push(Args&&... args)
	{
		auto  _old = m_head.load();
		auto* item = new lf_queue_item<T>(_old & 1 ? nullptr : reinterpret_cast<lf_queue_item<T>*>(_old), std::forward<Args>(args)...);

		while (!m_head.compare_exchange(_old, reinterpret_cast<std::uint64_t>(item)))
		{
			item->m_link = _old & 1 ? nullptr : reinterpret_cast<lf_queue_item<T>*>(_old);
		}

		if (_old & 1)
		{
			lf_queue_base::imp_notify();
		}
	}

	// Withdraw the list, supports range-for loop: for (auto&& x : y.pop_all()) ...
	lf_queue_slice<T> pop_all()
	{
		lf_queue_slice<T> result;
		result.m_head = reverse();
		return result;
	}

	// Apply func(data) to each element, return the total length
	template <typename F>
	std::size_t apply(F&& func)
	{
		std::size_t count = 0;

		for (auto slice = pop_all(); slice; slice.pop_front())
		{
			std::invoke(std::forward<F>(func), *slice);
		}

		return count;
	}

	// apply() overload for callable template argument
	template <auto F>
	std::size_t apply()
	{
		std::size_t count = 0;

		for (auto slice = pop_all(); slice; slice.pop_front())
		{
			std::invoke(F, *slice);
		}

		return count;
	}
};

// Assignable lock-free thread-safe value of any type (memory-inefficient)
template <typename T>
class lf_value final
{
	atomic_t<lf_value*> m_head;

	T m_data;

public:
	template <typename... Args>
	explicit constexpr lf_value(Args&&... args)
	    : m_head(this)
	    , m_data(std::forward<Args>(args)...)
	{
	}

	~lf_value()
	{
		// All values are kept in the queue until the end
		for (lf_value* ptr = m_head.load(); ptr != this;)
		{
			delete std::exchange(ptr, std::exchange(ptr->m_head.raw(), ptr));
		}
	}

	// Get current head, allows to inspect old values
	[[nodiscard]] const lf_value* head() const
	{
		return m_head.load();
	}

	// Inspect the initial (oldest) value
	[[nodiscard]] const T& first() const
	{
		return m_data;
	}

	[[nodiscard]] const T& get() const
	{
		return m_head.load()->m_data;
	}

	[[nodiscard]] operator const T&() const
	{
		return m_head.load()->m_data;
	}

	// Construct new value in-place
	template <typename... Args>
	const T& assign(Args&&... args)
	{
		lf_value* val = new lf_value(std::forward<Args>(args)...);
		lf_value* old = m_head.load();

		do
		{
			val->m_head = old;
		}
		while (!m_head.compare_exchange(old, val));

		return val->m_data;
	}

	// Copy-assign new value
	const T& operator =(const T& value)
	{
		return assign(value);
	}

	// Move-assign new value
	const T& operator =(T&& value)
	{
		return assign(std::move(value));
	}
};
