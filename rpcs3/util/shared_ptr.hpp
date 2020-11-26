#pragma once

#include <cstdint>
#include <memory>
#include "atomic.hpp"

namespace stx
{
	// TODO
	template <typename T, typename U>
	constexpr bool is_same_ptr_v = true;

	template <typename T, typename U>
	constexpr bool is_same_ptr_cast_v = std::is_convertible_v<U, T> && is_same_ptr_v<T, U>;

	template <typename T>
	class single_ptr;

	template <typename T>
	class shared_ptr;

	template <typename T>
	class atomic_ptr;

	// Basic assumption of userspace pointer size
	constexpr uint c_ptr_size = 47;

	// Use lower 17 bits as atomic_ptr internal refcounter (pointer is shifted)
	constexpr uint c_ref_mask = 0x1ffff, c_ref_size = 17;

	struct shared_counter
	{
		// Stored destructor
		void (*destroy)(void* ptr);

		// Reference counter
		atomic_t<std::size_t> refs{0};
	};

	template <typename T>
	class unique_data
	{
	public:
		T data;

		template <typename... Args>
		explicit constexpr unique_data(Args&&... args) noexcept
			: data(std::forward<Args>(args)...)
		{
		}
	};

	template <typename T>
	class unique_data<T[]>
	{
		std::size_t count;
	};

	// Control block with data and reference counter
	template <typename T>
	class alignas(T) shared_data final : public shared_counter, public unique_data<T>
	{
	public:
		using data_type = T;

		template <typename... Args>
		explicit constexpr shared_data(Args&&... args) noexcept
			: shared_counter{}
			, unique_data<T>(std::forward<Args>(args)...)
		{
		}
	};

	template <typename T>
	class alignas(T) shared_data<T[]> final : public shared_counter, public unique_data<T>
	{
	public:
		using data_type = T;
	};

	// Simplified unique pointer (well, not simplified, std::unique_ptr is preferred)
	template <typename T>
	class single_ptr
	{
		std::remove_extent_t<T>* m_ptr{};

		shared_data<T>* d() const noexcept
		{
			// Shared counter, deleter, should be at negative offset
			return std::launder(static_cast<shared_data<T>*>(reinterpret_cast<unique_data<T>*>(m_ptr)));
		}

		template <typename U>
		friend class shared_ptr;

		template <typename U>
		friend class atomic_ptr;

	public:
		using pointer = T*;

		using element_type = std::remove_extent_t<T>;

		constexpr single_ptr() noexcept = default;

		constexpr single_ptr(std::nullptr_t) noexcept {}

		single_ptr(const single_ptr&) = delete;

		single_ptr(single_ptr&& r) noexcept
			: m_ptr(r.m_ptr)
		{
			r.m_ptr = nullptr;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U>>>
		single_ptr(single_ptr<U>&& r) noexcept
			: m_ptr(r.m_ptr)
		{
			r.m_ptr = nullptr;
		}

		~single_ptr()
		{
			reset();
		}

		single_ptr& operator=(const single_ptr&) = delete;

		single_ptr& operator=(std::nullptr_t) noexcept
		{
			reset();
		}

		single_ptr& operator=(single_ptr&& r) noexcept
		{
			m_ptr = r.m_ptr;
			r.m_ptr = nullptr;
			return *this;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U>>>
		single_ptr& operator=(single_ptr<U>&& r) noexcept
		{
			m_ptr = r.m_ptr;
			r.m_ptr = nullptr;
			return *this;
		}

		void reset() noexcept
		{
			if (m_ptr) [[likely]]
			{
				d()->destroy(d());
				m_ptr = nullptr;
			}
		}

		void swap(single_ptr& r) noexcept
		{
			std::swap(m_ptr, r.m_ptr);
		}

		element_type* get() const noexcept
		{
			return m_ptr;
		}

		decltype(auto) operator*() const noexcept
		{
			if constexpr (std::is_void_v<element_type>)
			{
				return;
			}
			else
			{
				return *m_ptr;
			}
		}

		element_type* operator->() const noexcept
		{
			return m_ptr;
		}

		decltype(auto) operator[](std::ptrdiff_t idx) const noexcept
		{
			if constexpr (std::is_void_v<element_type>)
			{
				return;
			}
			else if constexpr (std::is_array_v<T>)
			{
				return m_ptr[idx];
			}
			else
			{
				return *m_ptr;
			}
		}

		explicit constexpr operator bool() const noexcept
		{
			return m_ptr != nullptr;
		}
	};

	template <typename T, bool Init = true, typename... Args>
	static std::enable_if_t<!(std::is_unbounded_array_v<T>) && (Init || !sizeof...(Args)), single_ptr<T>> make_single(Args&&... args) noexcept
	{
		shared_data<T>* ptr = nullptr;

		if constexpr (Init)
		{
			ptr = new shared_data<T>(std::forward<Args>(args)...);
		}
		else
		{
			ptr = new shared_data<T>;
		}

		ptr->destroy = [](void* p)
		{
			delete static_cast<shared_data<T>*>(p);
		};

		single_ptr<T> r;
		reinterpret_cast<std::remove_extent_t<T>*&>(r) = &ptr->data;
		return r;
	}

	template <typename T, bool Init = true>
	static std::enable_if_t<std::is_unbounded_array_v<T>, single_ptr<T>> make_single(std::size_t count) noexcept
	{
		const std::size_t size = sizeof(shared_data<T>) + count * sizeof(std::remove_extent_t<T>);

		std::byte* bytes = nullptr;

		if constexpr (alignof(std::remove_extent_t<T>) > (__STDCPP_DEFAULT_NEW_ALIGNMENT__))
		{
			bytes = new (std::align_val_t{alignof(std::remove_extent_t<T>)}) std::byte[size];
		}
		else
		{
			bytes = new std::byte[size];
		}

		// Initialize control block
		shared_data<T>* ptr = new (reinterpret_cast<shared_data<T>*>(bytes)) shared_data<T>();

		// Initialize array next to the control block
		T arr = reinterpret_cast<T>(ptr + 1);

		if constexpr (Init)
		{
			std::uninitialized_value_construct_n(arr, count);
		}
		else
		{
			std::uninitialized_default_construct_n(arr, count);
		}

		ptr->m_count = count;

		ptr->destroy = [](void* p)
		{
			shared_data<T>* ptr = static_cast<shared_data<T>*>(p);

			std::destroy_n(std::launder(reinterpret_cast<T>(ptr + 1)), ptr->m_count);

			ptr->~shared_data<T>();

			if constexpr (alignof(std::remove_extent_t<T>) > (__STDCPP_DEFAULT_NEW_ALIGNMENT__))
			{
				::operator delete[](reinterpret_cast<std::byte*>(p), std::align_val_t{alignof(std::remove_extent_t<T>)});
			}
			else
			{
				delete[] reinterpret_cast<std::byte*>(p);
			}
		};

		single_ptr<T> r;
		reinterpret_cast<std::remove_extent_t<T>*&>(r) = std::launder(arr);
		return r;
	}

	// Simplified shared pointer
	template <typename T>
	class shared_ptr
	{
		std::remove_extent_t<T>* m_ptr{};

		shared_data<T>* d() const noexcept
		{
			// Shared counter, deleter, should be at negative offset
			return std::launder(static_cast<shared_data<T>*>(reinterpret_cast<unique_data<T>*>(m_ptr)));
		}

		template <typename U>
		friend class atomic_ptr;

	public:
		using pointer = T*;

		using element_type = std::remove_extent_t<T>;

		constexpr shared_ptr() noexcept = default;

		constexpr shared_ptr(std::nullptr_t) noexcept {}

		shared_ptr(const shared_ptr& r) noexcept
			: m_ptr(r.m_ptr)
		{
			if (m_ptr)
				d()->refs++;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U>>>
		shared_ptr(const shared_ptr<U>& r) noexcept
			: m_ptr(r.m_ptr)
		{
			if (m_ptr)
				d()->refs++;
		}

		shared_ptr(shared_ptr&& r) noexcept
			: m_ptr(r.m_ptr)
		{
			r.m_ptr = nullptr;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U>>>
		shared_ptr(shared_ptr<U>&& r) noexcept
			: m_ptr(r.m_ptr)
		{
			r.m_ptr = nullptr;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U>>>
		shared_ptr(single_ptr<U>&& r) noexcept
			: m_ptr(r.m_ptr)
		{
			r.m_ptr = nullptr;
		}

		~shared_ptr()
		{
			reset();
		}

		shared_ptr& operator=(const shared_ptr& r) noexcept
		{
			shared_ptr(r).swap(*this);
			return *this;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U>>>
		shared_ptr& operator=(const shared_ptr<U>& r) noexcept
		{
			shared_ptr(r).swap(*this);
			return *this;
		}

		shared_ptr& operator=(shared_ptr&& r) noexcept
		{
			shared_ptr(std::move(r)).swap(*this);
			return *this;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U>>>
		shared_ptr& operator=(shared_ptr<U>&& r) noexcept
		{
			shared_ptr(std::move(r)).swap(*this);
			return *this;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U>>>
		shared_ptr& operator=(single_ptr<U>&& r) noexcept
		{
			shared_ptr(std::move(r)).swap(*this);
			return *this;
		}

		// Set to null
		void reset() noexcept
		{
			if (m_ptr && !--d()->refs) [[unlikely]]
			{
				d()->destroy(d());
				m_ptr = nullptr;
			}
		}

		// Converts to unique (single) ptr if reference is 1, otherwise returns null. Nullifies self.
		explicit operator single_ptr<T>() && noexcept
		{
			if (m_ptr && !--d()->refs)
			{
				d()->refs.release(1);
				return {std::move(*this)};
			}

			m_ptr = nullptr;
			return {};
		}

		void swap(shared_ptr& r) noexcept
		{
			std::swap(this->m_ptr, r.m_ptr);
		}

		element_type* get() const noexcept
		{
			return m_ptr;
		}

		decltype(auto) operator*() const noexcept
		{
			if constexpr (std::is_void_v<element_type>)
			{
				return;
			}
			else
			{
				return *m_ptr;
			}
		}

		element_type* operator->() const noexcept
		{
			return m_ptr;
		}

		decltype(auto) operator[](std::ptrdiff_t idx) const noexcept
		{
			if constexpr (std::is_void_v<element_type>)
			{
				return;
			}
			else if constexpr (std::is_array_v<T>)
			{
				return m_ptr[idx];
			}
			else
			{
				return *m_ptr;
			}
		}

		std::size_t use_count() const noexcept
		{
			if (m_ptr)
			{
				return d()->refs;
			}
			else
			{
				return 0;
			}
		}

		explicit constexpr operator bool() const noexcept
		{
			return m_ptr != nullptr;
		}

		template <typename U, typename = decltype(static_cast<U*>(std::declval<T*>())), typename = std::enable_if_t<is_same_ptr_v<U, T>>>
		explicit operator shared_ptr<U>() const noexcept
		{
			if (m_ptr)
			{
				d()->refs++;
			}

			shared_ptr<U> r;
			r.m_ptr = m_ptr;
			return r;
		}
	};

	template <typename T, bool Init = true, typename... Args>
	static std::enable_if_t<!std::is_unbounded_array_v<T> && (!Init || !sizeof...(Args)), shared_ptr<T>> make_shared(Args&&... args) noexcept
	{
		return make_single<T, Init>(std::forward<Args>(args)...);
	}

	template <typename T, bool Init = true>
	static std::enable_if_t<std::is_unbounded_array_v<T>, shared_ptr<T>> make_shared(std::size_t count) noexcept
	{
		return make_single<T, Init>(count);
	}

	// Atomic simplified shared pointer
	template <typename T>
	class atomic_ptr
	{
		mutable atomic_t<uptr> m_val{0};

		static shared_data<T>* d(uptr val)
		{
			return std::launder(static_cast<shared_data<T>*>(reinterpret_cast<unique_data<T>*>(val >> c_ref_size)));
		}

		shared_data<T>* d() const noexcept
		{
			return d(m_val);
		}

	public:
		using pointer = T*;

		using element_type = std::remove_extent_t<T>;

		using shared_type = shared_ptr<T>;

		constexpr atomic_ptr() noexcept = default;

		constexpr atomic_ptr(std::nullptr_t) noexcept {}

		explicit atomic_ptr(T value) noexcept
		{
			auto r = make_single<T>(std::move(value));
			m_val = reinterpret_cast<uptr>(std::exchange(r.m_ptr, nullptr)) << c_ref_size;
			d()->refs += c_ref_mask;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U>>>
		atomic_ptr(const shared_ptr<U>& r) noexcept
			: m_val(reinterpret_cast<uptr>(r.m_ptr) << c_ref_size)
		{
			// Obtain a ref + as many refs as an atomic_ptr can additionally reference
			if (m_val)
				d()->refs += c_ref_mask + 1;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U>>>
		atomic_ptr(shared_ptr<U>&& r) noexcept
			: m_val(reinterpret_cast<uptr>(r.m_ptr) << c_ref_size)
		{
			r.m_ptr = nullptr;

			if (m_val)
				d()->refs += c_ref_mask;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U>>>
		atomic_ptr(single_ptr<U>&& r) noexcept
			: m_val(reinterpret_cast<uptr>(r.m_ptr) << c_ref_size)
		{
			r.m_ptr = nullptr;

			if (m_val)
				d()->refs += c_ref_mask;
		}

		~atomic_ptr()
		{
			const uptr v = m_val.raw();

			if (v && !d(v)->refs.sub_fetch(c_ref_mask + 1 - (v & c_ref_mask)))
			{
				d(v)->destroy(d(v));
			}
		}

		atomic_ptr& operator=(T value) noexcept
		{
			// TODO: does it make sense?
			store(make_single<T>(std::move(value)));
			return *this;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U>>>
		atomic_ptr& operator=(const shared_ptr<U>& r) noexcept
		{
			store(r);
			return *this;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U>>>
		atomic_ptr& operator=(shared_ptr<U>&& r) noexcept
		{
			store(std::move(r));
			return *this;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U>>>
		atomic_ptr& operator=(single_ptr<U>&& r) noexcept
		{
			store(std::move(r));
			return *this;
		}

		shared_type load() const noexcept
		{
			shared_type r;

			// Add reference
			const auto [prev, did_ref] = m_val.fetch_op([](uptr& val)
			{
				if (val >> c_ref_size)
				{
					val++;
					return true;
				}

				return false;
			});

			if (!did_ref)
			{
				// Null pointer
				return r;
			}

			// Set referenced pointer
			r.m_ptr = std::launder(reinterpret_cast<element_type*>(prev >> c_ref_size));

			// Dereference if same pointer
			m_val.fetch_op([prev = prev](uptr& val)
			{
				if (val >> c_ref_size == prev >> c_ref_size)
				{
					val--;
					return true;
				}

				return false;
			});

			return r;
		}

		void store(T value) noexcept
		{
			store(make_single<T>(std::move(value)));
		}

		void store(shared_type value) noexcept
		{
			if (value.m_ptr)
			{
				// Consume value and add refs
				value.d()->refs += c_ref_mask;
			}

			atomic_ptr old;
			old.m_val.raw() = m_val.exchange(reinterpret_cast<uptr>(std::exchange(value.m_ptr, nullptr)) << c_ref_size);
		}

		[[nodiscard]] shared_type exchange(shared_type value) noexcept
		{
			atomic_ptr old;

			if (value.m_ptr)
			{
				// Consume value and add refs
				value.d()->refs += c_ref_mask;
				old.m_val.raw() += 1;
			}

			old.m_val.raw() += m_val.exchange(reinterpret_cast<uptr>(std::exchange(value.m_ptr, nullptr)) << c_ref_size);

			shared_type r;
			r.m_ptr = old.m_val >> c_ref_size;
			return r;
		}

		// Simple atomic load is much more effective than load(), but it's a non-owning reference
		const volatile void* observe() const noexcept
		{
			return reinterpret_cast<const volatile void*>(m_val >> c_ref_size);
		}

		explicit constexpr operator bool() const noexcept
		{
			return m_val != 0;
		}

		bool is_equal(const shared_ptr<T>& r) const noexcept
		{
			return observe() == r.get();
		}

		bool is_equal(const single_ptr<T>& r) const noexcept
		{
			return observe() == r.get();
		}
	};
}

namespace std
{
	template <typename T>
	void swap(stx::single_ptr<T>& lhs, stx::single_ptr<T>& rhs) noexcept
	{
		lhs.swap(rhs);
	}

	template <typename T>
	void swap(stx::shared_ptr<T>& lhs, stx::shared_ptr<T>& rhs) noexcept
	{
		lhs.swap(rhs);
	}
}

using stx::single_ptr;
using stx::shared_ptr;
using stx::atomic_ptr;
using stx::make_single;
