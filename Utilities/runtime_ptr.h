#pragma once


enum class object_storage : std::uint8_t
{
	stack,
	heap
};

template<typename Type>
class runtime_ptr
{
public:
	using type = Type;

private:
	type *m_ptr = nullptr;
	object_storage m_storage = object_storage::stack;
	mutable std::shared_ptr<type> m_shared_ptr;

public:
	runtime_ptr() = default;

	runtime_ptr(type* ptr, object_storage storage = object_storage::stack, std::shared_ptr<type> shared_ptr = nullptr)
		: m_ptr(ptr)
		, m_shared_ptr(shared_ptr)
		, m_storage(storage)
	{
	}

	runtime_ptr(std::shared_ptr<type> shared_ptr)
		: m_shared_ptr(shared_ptr)
		, m_ptr(shared_ptr.get())
		, m_storage(object_storage::heap)
	{
	}

	runtime_ptr(std::unique_ptr<type> unique_ptr)
		: m_ptr(unique_ptr.release())
		, m_storage(object_storage::heap)
	{
	}

	runtime_ptr(const runtime_ptr& rhs) noexcept
	{
		assign(rhs);
	}

	runtime_ptr(runtime_ptr&& rhs) noexcept
	{
		*this = std::move(rhs);
	}

	template<typename RhsType>
	operator runtime_ptr<RhsType>() const
	{
		share();

		return
		{
			const_cast<RhsType*>(static_cast<std::remove_cv_t<RhsType>*>(m_ptr)),
			m_storage,
			std::const_pointer_cast<RhsType>(std::static_pointer_cast<std::remove_cv_t<RhsType>>(m_shared_ptr))
		};
	}

	~runtime_ptr()
	{
		remove();
	}

	void assign(const runtime_ptr& rhs)
	{
		*this = rhs.share();
	}

	void swap(runtime_ptr& rhs) noexcept
	{
		using std::swap;

		swap(m_ptr, rhs.m_ptr);
		swap(m_shared_ptr, rhs.m_shared_ptr);
		swap(m_storage, rhs.m_storage);
	}

	void clear()
	{
		remove();

		m_ptr = nullptr;
		m_shared_ptr.reset();
	}

	void remove()
	{
		if (m_storage == object_storage::heap && !shared())
		{
			std::default_delete<type>()(m_ptr);
		}
	}

	runtime_ptr copy() const
	{
		runtime_ptr result(new type(*m_ptr), object_storage::heap);
		return result;
	}

	bool shared() const
	{
		return !!m_shared_ptr;
	}

	type* const get() noexcept
	{
		return m_ptr;
	}

	const type* const get() const noexcept
	{
		return m_ptr;
	}

	type* const operator ->()
	{
		return get();
	}

	const type* const operator ->() const
	{
		return get();
	}

	type& operator *()
	{
		return *get();
	}

	const type& operator *() const
	{
		return *get();
	}

	bool operator ==(const runtime_ptr& rhs) const
	{
		return m_ptr == rhs.m_ptr;
	}

	bool operator !=(const runtime_ptr& rhs) const
	{
		return m_ptr != rhs.m_ptr;
	}

	bool operator ==(nullptr_t) const
	{
		return m_ptr == nullptr;
	}

	bool operator !=(nullptr_t) const
	{
		return m_ptr != nullptr;
	}

	runtime_ptr& operator =(const runtime_ptr& rhs) noexcept = default;

	runtime_ptr& operator =(nullptr_t)
	{
		clear();
	}

	runtime_ptr& operator =(runtime_ptr&& rhs) noexcept
	{
		swap(rhs);

		return *this;
	}

	explicit operator bool() const
	{
		return *this == nullptr;
	}

private:
	void share() const
	{
		if (m_storage == object_storage::heap && !shared())
		{
			m_shared_ptr.reset(m_ptr);
		}
	}
};

template<typename Type>
struct is_unique_ptr : std::integral_constant<bool, false>
{
};

template<typename Type>
struct is_unique_ptr<std::unique_ptr<Type>> : std::integral_constant<bool, true>
{
};

template<typename Type>
struct is_shared_ptr : std::integral_constant<bool, false>
{
};

template<typename Type>
struct is_shared_ptr<std::shared_ptr<Type>> : std::integral_constant<bool, true>
{
};

template<typename Type>
struct is_runtime_ptr : std::integral_constant<bool, false>
{
};

template<typename Type>
struct is_runtime_ptr<runtime_ptr<Type>> : std::integral_constant<bool, true>
{
};

template<typename Type>
struct is_smart_pointer : std::integral_constant<bool, is_unique_ptr<Type>::value || is_shared_ptr<Type>::value || is_runtime_ptr<Type>::value>
{
};

template<typename Type>
struct is_any_pointer : std::integral_constant<bool, std::is_pointer<Type>::value || is_smart_pointer<Type>::value>
{
};

