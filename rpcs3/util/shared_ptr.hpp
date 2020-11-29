#pragma once // No BOM and only basic ASCII in this header, or a neko will die

#include <cstdint>
#include <memory>
#include "atomic.hpp"

namespace stx
{
	template <typename T, typename U>
	constexpr bool is_same_ptr() noexcept
	{
		// I would like to make it a trait if there is some trick.
		// And believe it shall possible with constexpr bit_cast.
		// Otherwise I hope it will compile in null code anyway.
		const auto u = reinterpret_cast<U*>(0x11223344556);
		const volatile void* x = u;
		return static_cast<T*>(u) == x;
	}

	// TODO
	template <typename T, typename U>
	constexpr bool is_same_ptr_v = true;

	template <typename T, typename U>
	constexpr bool is_same_ptr_cast_v = std::is_same_v<T, U> || std::is_convertible_v<U, T> && is_same_ptr_v<T, U>;

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
		void (*destroy)(shared_counter* _this);

		// Reference counter
		atomic_t<std::size_t> refs{1};
	};

	template <std::size_t Size, std::size_t Align, typename = void>
	struct align_filler
	{
	};

	template <std::size_t Size, std::size_t Align>
	struct align_filler<Size, Align, std::enable_if_t<(Align > Size)>>
	{
		char dummy[Align - Size];
	};

	// Control block with data and reference counter
	template <typename T>
	class alignas(T) shared_data final : align_filler<sizeof(shared_counter), alignof(T)>
	{
	public:
		shared_counter m_ctr;

		T m_data;

		template <typename... Args>
		explicit constexpr shared_data(Args&&... args) noexcept
			: m_ctr{}
			, m_data(std::forward<Args>(args)...)
		{
		}
	};

	template <typename T>
	class alignas(T) shared_data<T[]> final : align_filler<sizeof(shared_counter) + sizeof(std::size_t), alignof(T)>
	{
	public:
		std::size_t m_count;

		shared_counter m_ctr;

		constexpr shared_data() noexcept = default;
	};

	// Simplified unique pointer. Wwell, not simplified, std::unique_ptr is preferred.
	// This one is shared_ptr counterpart, it has a control block with refs = 1.
	template <typename T>
	class single_ptr
	{
		std::remove_extent_t<T>* m_ptr{};

		shared_counter* d() const noexcept
		{
			// Shared counter, deleter, should be at negative offset
			return std::launder(reinterpret_cast<shared_counter*>(reinterpret_cast<u64>(m_ptr) - sizeof(shared_counter)));
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
			verify(HERE), is_same_ptr<T, U>();
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
			verify(HERE), is_same_ptr<T, U>();
			m_ptr = r.m_ptr;
			r.m_ptr = nullptr;
			return *this;
		}

		void reset() noexcept
		{
			if (m_ptr) [[likely]]
			{
				const auto o = d();
				o->destroy(o);
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

		// "Moving" "static cast"
		template <typename U, typename = decltype(static_cast<U*>(std::declval<T*>())), typename = std::enable_if_t<is_same_ptr_v<U, T>>>
		explicit operator single_ptr<U>() && noexcept
		{
			verify(HERE), is_same_ptr<U, T>();

			single_ptr<U> r;
			r.m_ptr = static_cast<decltype(r.m_ptr)>(std::exchange(m_ptr, nullptr));
			return r;
		}
	};

	template <typename T, bool Init = true, typename... Args>
	static std::enable_if_t<!(std::is_unbounded_array_v<T>) && (Init || !sizeof...(Args)), single_ptr<T>> make_single(Args&&... args) noexcept
	{
		static_assert(offsetof(shared_data<T>, m_data) - offsetof(shared_data<T>, m_ctr) == sizeof(shared_counter));

		using etype = std::remove_extent_t<T>;

		shared_data<T>* ptr = nullptr;

		if constexpr (Init && !std::is_array_v<T>)
		{
			ptr = new shared_data<T>(std::forward<Args>(args)...);
		}
		else
		{
			ptr = new shared_data<T>;

			if constexpr (Init && std::is_array_v<T>)
			{
				// Weird case, destroy and reinitialize every fixed array arg (fill)
				for (auto& e : ptr->m_data)
				{
					e.~etype();
					new (&e) etype(std::forward<Args>(args)...);
				}
			}
		}

		ptr->m_ctr.destroy = [](shared_counter* _this)
		{
			delete reinterpret_cast<shared_data<T>*>(reinterpret_cast<u64>(_this) - offsetof(shared_data<T>, m_ctr));
		};

		single_ptr<T> r;

		if constexpr (std::is_array_v<T>)
		{
			reinterpret_cast<etype*&>(r) = +ptr->m_data;
		}
		else
		{
			reinterpret_cast<etype*&>(r) = &ptr->m_data;
		}

		return r;
	}

	template <typename T, bool Init = true>
	static std::enable_if_t<std::is_unbounded_array_v<T>, single_ptr<T>> make_single(std::size_t count) noexcept
	{
		static_assert(sizeof(shared_data<T>) - offsetof(shared_data<T>, m_ctr) == sizeof(shared_counter));

		using etype = std::remove_extent_t<T>;

		const std::size_t size = sizeof(shared_data<T>) + count * sizeof(etype);

		std::byte* bytes = nullptr;

		if constexpr (alignof(etype) > (__STDCPP_DEFAULT_NEW_ALIGNMENT__))
		{
			bytes = new (std::align_val_t{alignof(etype)}) std::byte[size];
		}
		else
		{
			bytes = new std::byte[size];
		}

		// Initialize control block
		shared_data<T>* ptr = new (reinterpret_cast<shared_data<T>*>(bytes)) shared_data<T>();

		// Initialize array next to the control block
		etype* arr = reinterpret_cast<etype*>(bytes + sizeof(shared_data<T>));

		if constexpr (Init)
		{
			std::uninitialized_value_construct_n(arr, count);
		}
		else
		{
			std::uninitialized_default_construct_n(arr, count);
		}

		ptr->m_count = count;

		ptr->m_ctr.destroy = [](shared_counter* _this)
		{
			shared_data<T>* ptr = reinterpret_cast<shared_data<T>*>(reinterpret_cast<u64>(_this) - offsetof(shared_data<T>, m_ctr));

			std::byte* bytes = reinterpret_cast<std::byte*>(ptr);

			std::destroy_n(std::launder(reinterpret_cast<etype*>(bytes + sizeof(shared_data<T>))), ptr->m_count);

			ptr->~shared_data<T>();

			if constexpr (alignof(etype) > (__STDCPP_DEFAULT_NEW_ALIGNMENT__))
			{
				::operator delete[](bytes, std::align_val_t{alignof(etype)});
			}
			else
			{
				delete[] bytes;
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

		shared_counter* d() const noexcept
		{
			// Shared counter, deleter, should be at negative offset
			return std::launder(reinterpret_cast<shared_counter*>(reinterpret_cast<u64>(m_ptr) - sizeof(shared_counter)));
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
			verify(HERE), is_same_ptr<T, U>();
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
			verify(HERE), is_same_ptr<T, U>();
			r.m_ptr = nullptr;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U>>>
		shared_ptr(single_ptr<U>&& r) noexcept
			: m_ptr(r.m_ptr)
		{
			verify(HERE), is_same_ptr<T, U>();
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
			verify(HERE), is_same_ptr<T, U>();
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
			verify(HERE), is_same_ptr<T, U>();
			shared_ptr(std::move(r)).swap(*this);
			return *this;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U>>>
		shared_ptr& operator=(single_ptr<U>&& r) noexcept
		{
			verify(HERE), is_same_ptr<T, U>();
			shared_ptr(std::move(r)).swap(*this);
			return *this;
		}

		// Set to null
		void reset() noexcept
		{
			const auto o = d();

			if (m_ptr && !--o->refs) [[unlikely]]
			{
				o->destroy(o);
				m_ptr = nullptr;
			}
		}

		// Converts to unique (single) ptr if reference is 1, otherwise returns null. Nullifies self.
		template <typename U, typename = decltype(static_cast<U*>(std::declval<T*>())), typename = std::enable_if_t<is_same_ptr_v<U, T>>>
		explicit operator single_ptr<U>() && noexcept
		{
			verify(HERE), is_same_ptr<U, T>();

			const auto o = d();

			if (m_ptr && !--o->refs)
			{
				// Convert last reference to single_ptr instance.
				o->refs.release(1);
				single_ptr<T> r;
				r.m_ptr = static_cast<decltype(r.m_ptr)>(std::exchange(m_ptr, nullptr));
				return r;
			}

			// Otherwise, both pointers are gone. Didn't seem right to do it in the constructor.
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

		// Basic "static cast" support
		template <typename U, typename = decltype(static_cast<U*>(std::declval<T*>())), typename = std::enable_if_t<is_same_ptr_v<U, T>>>
		explicit operator shared_ptr<U>() const& noexcept
		{
			verify(HERE), is_same_ptr<U, T>();

			if (m_ptr)
			{
				d()->refs++;
			}

			shared_ptr<U> r;
			r.m_ptr = static_cast<decltype(r.m_ptr)>(m_ptr);
			return r;
		}

		// "Moving" "static cast"
		template <typename U, typename = decltype(static_cast<U*>(std::declval<T*>())), typename = std::enable_if_t<is_same_ptr_v<U, T>>>
		explicit operator shared_ptr<U>() && noexcept
		{
			verify(HERE), is_same_ptr<U, T>();

			shared_ptr<U> r;
			r.m_ptr = static_cast<decltype(r.m_ptr)>(std::exchange(m_ptr, nullptr));
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

		static shared_counter* d(uptr val)
		{
			return std::launder(reinterpret_cast<shared_counter*>((val >> c_ref_size) - sizeof(shared_counter)));
		}

		shared_counter* d() const noexcept
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
			verify(HERE), is_same_ptr<T, U>();

			// Obtain a ref + as many refs as an atomic_ptr can additionally reference
			if (m_val)
				d()->refs += c_ref_mask + 1;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U>>>
		atomic_ptr(shared_ptr<U>&& r) noexcept
			: m_val(reinterpret_cast<uptr>(r.m_ptr) << c_ref_size)
		{
			verify(HERE), is_same_ptr<T, U>();
			r.m_ptr = nullptr;

			if (m_val)
				d()->refs += c_ref_mask;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U>>>
		atomic_ptr(single_ptr<U>&& r) noexcept
			: m_val(reinterpret_cast<uptr>(r.m_ptr) << c_ref_size)
		{
			verify(HERE), is_same_ptr<T, U>();
			r.m_ptr = nullptr;

			if (m_val)
				d()->refs += c_ref_mask;
		}

		~atomic_ptr()
		{
			const uptr v = m_val.raw();
			const auto o = d(v);

			if (v >> c_ref_size && !o->refs.sub_fetch(c_ref_mask + 1 - (v & c_ref_mask)))
			{
				o->destroy(o);
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
			verify(HERE), is_same_ptr<T, U>();
			store(r);
			return *this;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U>>>
		atomic_ptr& operator=(shared_ptr<U>&& r) noexcept
		{
			verify(HERE), is_same_ptr<T, U>();
			store(std::move(r));
			return *this;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U>>>
		atomic_ptr& operator=(single_ptr<U>&& r) noexcept
		{
			verify(HERE), is_same_ptr<T, U>();
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
			r.d()->refs++;

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
			if (value.m_ptr)
			{
				// Consume value and add refs
				value.d()->refs += c_ref_mask;
			}

			atomic_ptr old;
			old.m_val.raw() += m_val.exchange(reinterpret_cast<uptr>(std::exchange(value.m_ptr, nullptr)) << c_ref_size);

			shared_type r;
			r.m_ptr = std::launder(reinterpret_cast<element_type*>(old.m_val >> c_ref_size));

			if (old.m_val.raw())
			{
				old.m_val.raw()++;
			}

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
