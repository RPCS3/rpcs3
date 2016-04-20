#pragma once

#include "Atomic.h"
#include <memory>

// Unfinished. Only std::default_delete will work as expected.
template<typename T, typename D>
class atomic_ptr_base : D
{
protected:
	atomic_t<T*> m_ptr;

	constexpr atomic_ptr_base(T* ptr)
		: m_ptr(ptr)
	{
	}

public:
	~atomic_ptr_base()
	{
		if (m_ptr)
		{
			(*this)(m_ptr.load());
		}
	}

	D& get_deleter()
	{
		return *this;
	}

	const D& get_deleter() const
	{
		return *this;
	}
};

// Simple atomic pointer with unique ownership. Draft, unfinished.
template<typename T, typename D = std::default_delete<T>>
class atomic_ptr final : atomic_ptr_base<T, D>
{
	using base = atomic_ptr_base<T, D>;

	static_assert(sizeof(T*) == sizeof(base), "atomic_ptr<> error: invalid deleter (empty class expected)");

public:
	constexpr atomic_ptr()
		: base(nullptr)
	{
	}

	constexpr atomic_ptr(std::nullptr_t)
		: base(nullptr)
	{
	}

	explicit atomic_ptr(T* ptr)
		: base(ptr)
	{
	}

	template<typename T2, typename = std::enable_if_t<std::is_convertible<T2, T>::value>>
	atomic_ptr(std::unique_ptr<T2, D>&& ptr)
		: base(ptr.release())
	{
	}

	atomic_ptr& operator =(std::nullptr_t)
	{
		if (T* old = base::m_ptr.exchange(nullptr))
		{
			this->get_deleter()(old);
		}

		return *this;
	}

	template<typename T2, typename = std::enable_if_t<std::is_convertible<T2, T>::value>>
	atomic_ptr& operator =(std::unique_ptr<T2, D>&& ptr)
	{
		if (T* old = base::m_ptr.exchange(ptr.release()))
		{
			this->get_deleter()(old);
		}

		return *this;
	}

	void swap(std::unique_ptr<T, D>& ptr)
	{
		ptr.reset(base::m_ptr.exchange(ptr.release()));
	}

	std::add_lvalue_reference_t<T> operator *() const
	{
		return *base::m_ptr;
	}

	T* operator ->() const
	{
		return base::m_ptr;
	}

	T* get() const
	{
		return base::m_ptr;
	}

	explicit operator bool() const
	{
		return base::m_ptr != nullptr;
	}

	T* release() const
	{
		return base::m_ptr.exchange(0);
	}

	void reset(T* ptr = nullptr)
	{
		if (T* old = base::m_ptr.exchange(ptr))
		{
			this->get_deleter()(old);
		}
	}

	// Steal the pointer from `ptr`, convert old value to unique_ptr
	std::unique_ptr<T, D> exchange(std::unique_ptr<T, D>&& ptr)
	{
		return std::unique_ptr<T, D>(base::m_ptr.exchange(ptr.release()));
	}

	// If pointer is null, steal it from `ptr`
	bool test_and_swap(std::unique_ptr<T, D>&& ptr)
	{
		if (base::m_ptr.compare_and_swap_test(nullptr, ptr.get()))
		{
			ptr.release();
			return true;
		}

		return false;
	}
};

template<typename T, typename D>
class atomic_ptr<T[], D> final : atomic_ptr_base<T[], D>
{
	// TODO
};
