#pragma once

#include "types.h"
#include "util/atomic.hpp"

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
		for (auto ptr = m_next.raw(); ptr;)
		{
			delete std::exchange(ptr, std::exchange(ptr->m_next.raw(), nullptr));
		}
	}

	T& operator [](std::size_t index)
	{
		if (index < N) [[likely]]
		{
			return m_data[index];
		}
		else if (!m_next) [[unlikely]]
		{
			// Create new array block. It's not a full-fledged once-synchronization, unlikely needed.
			for (auto _new = new lf_array, ptr = this; ptr;)
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
	// LSB 32-bit: push, MSB 32-bit: pop
	atomic_t<u64> m_ctrl{};

public:
	constexpr lf_fifo() = default;

	// Get number of elements in the queue
	u32 size() const
	{
		const u64 ctrl = m_ctrl.load();
		return static_cast<u32>(ctrl - (ctrl >> 32));
	}

	// Acquire the place for one or more elements.
	u32 push_begin(u32 count = 1)
	{
		return static_cast<u32>(m_ctrl.fetch_add(count));
	}

	// Get current "pop" position
	u32 peek() const
	{
		return static_cast<u32>(m_ctrl >> 32);
	}

	// Acknowledge processed element, return number of the next one.
	// Perform clear if possible, zero is returned in this case.
	u32 pop_end(u32 count = 1)
	{
		return m_ctrl.atomic_op([&](u64& ctrl)
		{
			ctrl += u64{count} << 32;

			if (ctrl >> 32 == static_cast<u32>(ctrl))
			{
				// Clean if possible
				ctrl = 0;
			}

			return static_cast<u32>(ctrl >> 32);
		});
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

	template <typename U>
	friend class lf_bunch;

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

	template <typename U>
	friend class lf_bunch;

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

// Linked list-based multi-producer queue (the consumer drains the whole queue at once)
template <typename T>
class lf_queue final
{
	atomic_t<lf_queue_item<T>*> m_head{nullptr};

	// Extract all elements and reverse element order (FILO to FIFO)
	lf_queue_item<T>* reverse() noexcept
	{
		if (auto* head = m_head.load() ? m_head.exchange(nullptr) : nullptr)
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
		delete m_head.load();
	}

	void wait() noexcept
	{
		if (m_head == nullptr)
		{
			m_head.wait(nullptr);
		}
	}

	template <typename... Args>
	void push(Args&&... args)
	{
		auto _old = m_head.load();
		auto item = new lf_queue_item<T>(_old, std::forward<Args>(args)...);

		while (!m_head.compare_exchange(_old, item))
		{
			item->m_link = _old;
		}

		if (!_old)
		{
			// Notify only if queue was empty
			m_head.notify_one();
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
	std::size_t apply(F func)
	{
		std::size_t count = 0;

		for (auto slice = pop_all(); slice; slice.pop_front())
		{
			std::invoke(func, *slice);
		}

		return count;
	}

	// Iterator that enables direct endless range-for loop: for (auto* ptr : queue) ...
	class iterator
	{
		lf_queue* _this = nullptr;

		lf_queue_slice<T> m_data;

	public:
		constexpr iterator() = default;

		explicit iterator(lf_queue* _this)
			: _this(_this)
		{
			m_data = _this->pop_all();
		}

		bool operator !=(const iterator& rhs) const
		{
			return _this != rhs._this;
		}

		T* operator *() const
		{
			return m_data ? m_data.get() : nullptr;
		}

		iterator& operator ++()
		{
			if (m_data)
			{
				m_data.pop_front();
			}

			if (!m_data)
			{
				m_data = _this->pop_all();

				if (!m_data)
				{
					_this->wait();
					m_data = _this->pop_all();
				}
			}

			return *this;
		}
	};

	iterator begin()
	{
		return iterator{this};
	}

	iterator end()
	{
		return iterator{};
	}
};

// Concurrent linked list, elements remain until destroyed.
template <typename T>
class lf_bunch final
{
	atomic_t<lf_queue_item<T>*> m_head{nullptr};

public:
	constexpr lf_bunch() noexcept = default;

	~lf_bunch()
	{
		delete m_head.load();
	}

	// Add unconditionally
	template <typename... Args>
	T* push(Args&&... args) noexcept
	{
		auto _old = m_head.load();
		auto item = new lf_queue_item<T>(_old, std::forward<Args>(args)...);

		while (!m_head.compare_exchange(_old, item))
		{
			item->m_link = _old;
		}

		return &item->m_data;
	}

	// Add if pred(item, all_items) is true for all existing items
	template <typename F, typename... Args>
	T* push_if(F pred, Args&&... args) noexcept
	{
		auto _old = m_head.load();
		auto _chk = _old;
		auto item = new lf_queue_item<T>(_old, std::forward<Args>(args)...);

		_chk = nullptr;

		do
		{
			item->m_link = _old;

			// Check all items in the queue
			for (auto ptr = _old; ptr != _chk; ptr = ptr->m_link)
			{
				if (!pred(item->m_data, ptr->m_data))
				{
					item->m_link = nullptr;
					delete item;
					return nullptr;
				}
			}

			// Set to not check already checked items
			_chk = _old;
		}
		while (!m_head.compare_exchange(_old, item));

		return &item->m_data;
	}

	lf_queue_iterator<T> begin() const
	{
		lf_queue_iterator<T> result;
		result.m_ptr = m_head.load();
		return result;
	}

	lf_queue_iterator<T> end() const
	{
		return {};
	}
};
