#pragma once // No BOM and only basic ASCII in this header, or a neko will die

#include <cstdint>
#include <memory>
#include "atomic.hpp"

namespace stx
{
	namespace detail
	{
		template <typename T>
		union fake_t
		{
			char dummy;
			T data;

			fake_t() noexcept {}
			~fake_t() {}
		};

		template <typename T>
#ifdef _MSC_VER
		static const
#else
		static thread_local const
#endif
		fake_t<std::remove_cv_t<T>> sample{};
	}

	// Classify compile-time information available for pointers
	enum class same_ptr
	{
		no,
		yes,
		maybe
	};

	template <typename X, typename Y>
	constexpr bool is_same_ptr_test(const volatile Y* ptr = std::addressof(detail::sample<Y>.data))
	{
		return static_cast<const volatile X*>(ptr) == static_cast<const volatile void*>(ptr);
	}

	// Checks whether the cast between two types is the same pointer
	template <typename T, typename U>
	constexpr same_ptr is_same_ptr() noexcept
	{
		if constexpr (std::is_void_v<T> || std::is_void_v<U> || std::is_same_v<std::remove_cv_t<T>, std::remove_cv_t<U>>)
		{
			return same_ptr::yes;
		}
		else if constexpr (PtrCastable<T, U> && !std::is_abstract_v<U>)
		{
			return is_same_ptr_test<T, U>() ? same_ptr::yes : same_ptr::no;
		}
		else if constexpr (PtrCastable<T, U> && !std::is_abstract_v<T>)
		{
			return is_same_ptr_test<T, U>() ? same_ptr::yes : same_ptr::no;
		}

		return same_ptr::maybe;
	}

	template <typename T, typename U>
	constexpr same_ptr is_same_ptr_cast_v = std::is_convertible_v<U*, T*> ? is_same_ptr<T, U>() : same_ptr::no;

	template <typename T>
	class single_ptr;

	template <typename T>
	class shared_ptr;

	template <typename T>
	class atomic_ptr;

	// Basic assumption of userspace pointer size
	constexpr uint c_ptr_size = 47;

	// Use lower 17 bits as atomic_ptr internal counter of borrowed refs (pointer itself is shifted)
	constexpr uint c_ref_mask = 0x1ffff, c_ref_size = 17;

	// Remaining pointer bits
	constexpr uptr c_ptr_mask = static_cast<uptr>(-1) << c_ref_size;

	struct shared_counter
	{
		// Stored destructor
		atomic_t<void (*)(shared_counter* _this) noexcept> destroy{};

		// Reference counter
		atomic_t<usz> refs{1};
	};

	template <usz Size, usz Align>
	struct align_filler
	{
	};

	template <usz Size, usz Align> requires (Align > Size)
	struct align_filler<Size, Align>
	{
		char dummy[Align - Size];
	};

	// Control block with data and reference counter
	template <typename T>
	class alignas(T) shared_data final : align_filler<sizeof(shared_counter), alignof(T)>
	{
	public:
		shared_counter m_ctr{};

		T m_data;

		template <typename... Args>
		explicit constexpr shared_data(Args&&... args) noexcept
			: m_data(std::forward<Args>(args)...)
		{
		}
	};

	template <typename T>
	class alignas(T) shared_data<T[]> final : align_filler<sizeof(shared_counter) + sizeof(usz), alignof(T)>
	{
	public:
		usz m_count{};

		shared_counter m_ctr{};

		constexpr shared_data() noexcept = default;
	};

	// Simplified unique pointer. In some cases, std::unique_ptr is preferred.
	// This one is shared_ptr counterpart, it has a control block with refs and deleter.
	// It's trivially convertible to shared_ptr, and back if refs == 1.
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
		friend class single_ptr;

		template <typename U>
		friend class shared_ptr;

		template <typename U>
		friend class atomic_ptr;

	public:
		using pointer = T*;

		using element_type = std::remove_extent_t<T>;

		constexpr single_ptr() noexcept = default;

		single_ptr(const single_ptr&) = delete;

		// Default constructor or null_ptr should be used instead
		[[deprecated("Use null_ptr")]] single_ptr(std::nullptr_t) = delete;

		explicit single_ptr(shared_data<T>&, pointer ptr) noexcept
			: m_ptr(ptr)
		{
		}

		single_ptr(single_ptr&& r) noexcept
			: m_ptr(r.m_ptr)
		{
			r.m_ptr = nullptr;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U> != same_ptr::no>>
		single_ptr(single_ptr<U>&& r) noexcept
		{
			if constexpr (is_same_ptr<T, U>() == same_ptr::maybe)
				ensure(is_same_ptr_test<T, U>(r.m_ptr));

			m_ptr = r.m_ptr;
			r.m_ptr = nullptr;
		}

		~single_ptr()
		{
			reset();
		}

		single_ptr& operator=(const single_ptr&) = delete;

		[[deprecated("Use null_ptr")]] single_ptr& operator=(std::nullptr_t) = delete;

		single_ptr& operator=(single_ptr&& r) noexcept
		{
			single_ptr(std::move(r)).swap(*this);
			return *this;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U> != same_ptr::no>>
		single_ptr& operator=(single_ptr<U>&& r) noexcept
		{
			if constexpr (is_same_ptr<T, U>() == same_ptr::maybe)
				ensure(is_same_ptr_test<T, U>(r.m_ptr));

			single_ptr(std::move(r)).swap(*this);
			return *this;
		}

		void reset() noexcept
		{
			if (m_ptr) [[likely]]
			{
				const auto o = d();
				o->destroy.load()(o);
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

		decltype(auto) operator*() const noexcept requires (!std::is_void_v<element_type>)
		{
			return *m_ptr;
		}

		element_type* operator->() const noexcept
		{
			return m_ptr;
		}

		decltype(auto) operator[](std::ptrdiff_t idx) const noexcept requires (!std::is_void_v<element_type>)
		{
			if constexpr (std::is_array_v<T>)
			{
				return m_ptr[idx];
			}
			else
			{
				return *m_ptr;
			}
		}

		template <typename... Args> requires (std::is_invocable_v<T, Args&&...>)
		decltype(auto) operator()(Args&&... args) const noexcept
		{
			return std::invoke(*m_ptr, std::forward<Args>(args)...);
		}

		operator element_type*() const noexcept
		{
			return m_ptr;
		}

		explicit constexpr operator bool() const noexcept
		{
			return m_ptr != nullptr;
		}

		// "Moving" "static cast"
		template <typename U, typename = decltype(static_cast<U*>(std::declval<T*>())), typename = std::enable_if_t<is_same_ptr<U, T>() != same_ptr::no>>
		explicit operator single_ptr<U>() && noexcept
		{
			if constexpr (is_same_ptr<U, T>() == same_ptr::maybe)
				ensure(is_same_ptr_test<U, T>(m_ptr));

			single_ptr<U> r;
			r.m_ptr = static_cast<decltype(r.m_ptr)>(std::exchange(m_ptr, nullptr));
			return r;
		}
	};

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif

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

		ptr->m_ctr.destroy.raw() = [](shared_counter* _this) noexcept
		{
			delete reinterpret_cast<shared_data<T>*>(reinterpret_cast<u64>(_this) - offsetof(shared_data<T>, m_ctr));
		};

		return single_ptr<T>(*ptr, &ptr->m_data);
	}

	template <typename T, bool Init = true>
	static std::enable_if_t<std::is_unbounded_array_v<T>, single_ptr<T>> make_single(usz count) noexcept
	{
		static_assert(sizeof(shared_data<T>) - offsetof(shared_data<T>, m_ctr) == sizeof(shared_counter));

		using etype = std::remove_extent_t<T>;

		const usz size = sizeof(shared_data<T>) + count * sizeof(etype);

		std::byte* bytes = nullptr;

		if constexpr (alignof(etype) > (__STDCPP_DEFAULT_NEW_ALIGNMENT__))
		{
			bytes = static_cast<std::byte*>(::operator new(size, std::align_val_t{alignof(etype)}));
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

		ptr->m_ctr.destroy.raw() = [](shared_counter* _this) noexcept
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

		return single_ptr<T>(*ptr, std::launder(arr));
	}

	template <typename T>
	static single_ptr<std::remove_reference_t<T>> make_single_value(T&& value)
	{
		return make_single<std::remove_reference_t<T>>(std::forward<T>(value));
	}

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

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
		friend class shared_ptr;

		template <typename U>
		friend class atomic_ptr;

	public:
		using pointer = T*;

		using element_type = std::remove_extent_t<T>;

		constexpr shared_ptr() noexcept = default;

		shared_ptr(const shared_ptr& r) noexcept
			: m_ptr(r.m_ptr)
		{
			if (m_ptr)
				d()->refs++;
		}

		// Default constructor or null_ptr constant should be used instead
		[[deprecated("Use null_ptr")]] shared_ptr(std::nullptr_t) = delete;

		// Not-so-aliasing constructor: emulates std::enable_shared_from_this without its overhead
		explicit shared_ptr(T* _this) noexcept
			: m_ptr(_this)
		{
			// Random checks which may fail on invalid pointer
			ensure((reinterpret_cast<u64>(d()->destroy) - 0x10000) >> 47 == 0);
			ensure((d()->refs++ - 1) >> 58 == 0);
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U> != same_ptr::no>>
		shared_ptr(const shared_ptr<U>& r) noexcept
		{
			if constexpr (is_same_ptr<T, U>() == same_ptr::maybe)
				ensure(is_same_ptr_test<T, U>(r.m_ptr));

			m_ptr = r.m_ptr;
			if (m_ptr)
				d()->refs++;
		}

		shared_ptr(shared_ptr&& r) noexcept
			: m_ptr(r.m_ptr)
		{
			r.m_ptr = nullptr;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U> != same_ptr::no>>
		shared_ptr(shared_ptr<U>&& r) noexcept
		{
			if constexpr (is_same_ptr<T, U>() == same_ptr::maybe)
				ensure(is_same_ptr_test<T, U>(r.m_ptr));

			m_ptr = r.m_ptr;
			r.m_ptr = nullptr;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U> != same_ptr::no>>
		shared_ptr(single_ptr<U>&& r) noexcept
		{
			if constexpr (is_same_ptr<T, U>() == same_ptr::maybe)
				ensure(is_same_ptr_test<T, U>(r.m_ptr));

			m_ptr = r.m_ptr;
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

		[[deprecated("Use null_ptr")]] shared_ptr& operator=(std::nullptr_t) = delete;

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U> != same_ptr::no>>
		shared_ptr& operator=(const shared_ptr<U>& r) noexcept
		{
			if constexpr (is_same_ptr<T, U>() == same_ptr::maybe)
				ensure(is_same_ptr_test<T, U>(r.m_ptr));

			shared_ptr(r).swap(*this);
			return *this;
		}

		shared_ptr& operator=(shared_ptr&& r) noexcept
		{
			shared_ptr(std::move(r)).swap(*this);
			return *this;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U> != same_ptr::no>>
		shared_ptr& operator=(shared_ptr<U>&& r) noexcept
		{
			if constexpr (is_same_ptr<T, U>() == same_ptr::maybe)
				ensure(is_same_ptr_test<T, U>(r.m_ptr));

			shared_ptr(std::move(r)).swap(*this);
			return *this;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U> != same_ptr::no>>
		shared_ptr& operator=(single_ptr<U>&& r) noexcept
		{
			if constexpr (is_same_ptr<T, U>() == same_ptr::maybe)
				ensure(is_same_ptr_test<T, U>(r.m_ptr));

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
		template <typename U, typename = decltype(static_cast<U*>(std::declval<T*>())), typename = std::enable_if_t<is_same_ptr<U, T>() != same_ptr::no>>
		explicit operator single_ptr<U>() && noexcept
		{
			if constexpr (is_same_ptr<U, T>() == same_ptr::maybe)
				ensure(is_same_ptr_test<U, T>(m_ptr));

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

		decltype(auto) operator*() const noexcept requires (!std::is_void_v<element_type>)
		{
			return *m_ptr;
		}

		element_type* operator->() const noexcept
		{
			return m_ptr;
		}

		decltype(auto) operator[](std::ptrdiff_t idx) const noexcept requires (!std::is_void_v<element_type>)
		{
			if constexpr (std::is_array_v<T>)
			{
				return m_ptr[idx];
			}
			else
			{
				return *m_ptr;
			}
		}

		template <typename... Args> requires (std::is_invocable_v<T, Args&&...>)
		decltype(auto) operator()(Args&&... args) const noexcept
		{
			return std::invoke(*m_ptr, std::forward<Args>(args)...);
		}

		usz use_count() const noexcept
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

		operator element_type*() const noexcept
		{
			return m_ptr;
		}

		explicit constexpr operator bool() const noexcept
		{
			return m_ptr != nullptr;
		}

		// Basic "static cast" support
		template <typename U, typename = decltype(static_cast<U*>(std::declval<T*>())), typename = std::enable_if_t<is_same_ptr<U, T>() != same_ptr::no>>
		explicit operator shared_ptr<U>() const& noexcept
		{
			if constexpr (is_same_ptr<U, T>() == same_ptr::maybe)
				ensure(is_same_ptr_test<U, T>(m_ptr));

			if (m_ptr)
			{
				d()->refs++;
			}

			shared_ptr<U> r;
			r.m_ptr = static_cast<decltype(r.m_ptr)>(m_ptr);
			return r;
		}

		// "Moving" "static cast"
		template <typename U, typename = decltype(static_cast<U*>(std::declval<T*>())), typename = std::enable_if_t<is_same_ptr<U, T>() != same_ptr::no>>
		explicit operator shared_ptr<U>() && noexcept
		{
			if constexpr (is_same_ptr<U, T>() == same_ptr::maybe)
				ensure(is_same_ptr_test<U, T>(m_ptr));

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
	static std::enable_if_t<std::is_unbounded_array_v<T>, shared_ptr<T>> make_shared(usz count) noexcept
	{
		return make_single<T, Init>(count);
	}

	template <typename T>
	static shared_ptr<std::remove_reference_t<T>> make_shared_value(T&& value)
	{
		return make_single_value(std::forward<T>(value));
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

		template <typename U>
		friend class atomic_ptr;

	public:
		using pointer = T*;

		using element_type = std::remove_extent_t<T>;

		using shared_type = shared_ptr<T>;

		constexpr atomic_ptr() noexcept = default;

		// Optimized value construct
		template <typename... Args, typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
		explicit atomic_ptr(Args&&... args) noexcept
		{
			shared_type r = make_single<T>(std::forward<Args>(args)...);
			m_val = reinterpret_cast<uptr>(std::exchange(r.m_ptr, nullptr)) << c_ref_size;
			d()->refs.raw() += c_ref_mask;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U> != same_ptr::no>>
		atomic_ptr(const shared_ptr<U>& r) noexcept
		{
			if constexpr (is_same_ptr<T, U>() == same_ptr::maybe)
				ensure(is_same_ptr_test<T, U>(r.m_ptr));

			// Obtain a ref + as many refs as an atomic_ptr can additionally reference
			m_val = reinterpret_cast<uptr>(r.m_ptr) << c_ref_size;
			if (m_val)
				d()->refs += c_ref_mask + 1;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U> != same_ptr::no>>
		atomic_ptr(shared_ptr<U>&& r) noexcept
		{
			if constexpr (is_same_ptr<T, U>() == same_ptr::maybe)
				ensure(is_same_ptr_test<T, U>(r.m_ptr));

			m_val = reinterpret_cast<uptr>(r.m_ptr) << c_ref_size;
			r.m_ptr = nullptr;

			if (m_val)
				d()->refs += c_ref_mask;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U> != same_ptr::no>>
		atomic_ptr(single_ptr<U>&& r) noexcept
		{
			if constexpr (is_same_ptr<T, U>() == same_ptr::maybe)
				ensure(is_same_ptr_test<T, U>(r.m_ptr));

			m_val = reinterpret_cast<uptr>(r.m_ptr) << c_ref_size;
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
				o->destroy.load()(o);
			}
		}

		// Optimized value assignment
		atomic_ptr& operator=(std::remove_cv_t<T> value) noexcept
		{
			shared_type r = make_single<T>(std::move(value));
			r.d()->refs.raw() += c_ref_mask;

			atomic_ptr old;
			old.m_val.raw() = m_val.exchange(reinterpret_cast<uptr>(std::exchange(r.m_ptr, nullptr)) << c_ref_size);
			return *this;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U> != same_ptr::no>>
		atomic_ptr& operator=(const shared_ptr<U>& r) noexcept
		{
			if constexpr (is_same_ptr<T, U>() == same_ptr::maybe)
				ensure(is_same_ptr_test<T, U>(r.m_ptr));

			store(r);
			return *this;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U> != same_ptr::no>>
		atomic_ptr& operator=(shared_ptr<U>&& r) noexcept
		{
			if constexpr (is_same_ptr<T, U>() == same_ptr::maybe)
				ensure(is_same_ptr_test<T, U>(r.m_ptr));

			store(std::move(r));
			return *this;
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U> != same_ptr::no>>
		atomic_ptr& operator=(single_ptr<U>&& r) noexcept
		{
			if constexpr (is_same_ptr<T, U>() == same_ptr::maybe)
				ensure(is_same_ptr_test<T, U>(r.m_ptr));

			store(std::move(r));
			return *this;
		}

		void reset() noexcept
		{
			store(shared_type{});
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

			// Dereference if still the same pointer
			const auto [_, did_deref] = m_val.fetch_op([prev = prev](uptr& val)
			{
				if (val >> c_ref_size == prev >> c_ref_size)
				{
					val--;
					return true;
				}

				return false;
			});

			if (!did_deref)
			{
				// Otherwise fix ref count (atomic_ptr has been overwritten)
				r.d()->refs--;
			}

			return r;
		}

		// Atomically inspect pointer with the possibility to reference it if necessary
		template <typename F, typename RT = std::invoke_result_t<F, const shared_type&>>
		RT peek_op(F op) const noexcept
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

			// Set fake unreferenced pointer
			if (did_ref)
			{
				r.m_ptr = std::launder(reinterpret_cast<element_type*>(prev >> c_ref_size));
			}

			// Result temp storage
			[[maybe_unused]] std::conditional_t<std::is_void_v<RT>, int, RT> result;

			// Invoke
			if constexpr (std::is_void_v<RT>)
			{
				std::invoke(op, std::as_const(r));

				if (!did_ref)
				{
					return;
				}
			}
			else
			{
				result = std::invoke(op, std::as_const(r));

				if (!did_ref)
				{
					return result;
				}
			}

			// Dereference if still the same pointer
			const auto [_, did_deref] = m_val.fetch_op([prev = prev](uptr& val)
			{
				if (val >> c_ref_size == prev >> c_ref_size)
				{
					val--;
					return true;
				}

				return false;
			});

			if (did_deref)
			{
				// Deactivate fake pointer
				r.m_ptr = nullptr;
			}

			if constexpr (std::is_void_v<RT>)
			{
				return;
			}
			else
			{
				return result;
			}
		}

		template <typename... Args, typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
		void store(Args&&... args) noexcept
		{
			shared_type r = make_single<T>(std::forward<Args>(args)...);
			r.d()->refs.raw() += c_ref_mask;

			atomic_ptr old;
			old.m_val.raw() = m_val.exchange(reinterpret_cast<uptr>(std::exchange(r.m_ptr, nullptr)) << c_ref_size);
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

		template <typename... Args, typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
		[[nodiscard]] shared_type exchange(Args&&... args) noexcept
		{
			shared_type r = make_single<T>(std::forward<Args>(args)...);
			r.d()->refs.raw() += c_ref_mask;

			atomic_ptr old;
			old.m_val.raw() += m_val.exchange(reinterpret_cast<uptr>(r.m_ptr) << c_ref_size);
			old.m_val.raw() += 1;

			r.m_ptr = std::launder(reinterpret_cast<element_type*>(old.m_val >> c_ref_size));
			return r;
		}

		[[nodiscard]] shared_type exchange(shared_type value) noexcept
		{
			if (value.m_ptr)
			{
				// Consume value and add refs
				value.d()->refs += c_ref_mask;
			}

			atomic_ptr old;
			old.m_val.raw() += m_val.exchange(reinterpret_cast<uptr>(value.m_ptr) << c_ref_size);
			old.m_val.raw() += 1;

			value.m_ptr = std::launder(reinterpret_cast<element_type*>(old.m_val >> c_ref_size));
			return value;
		}

		// Ineffective
		[[nodiscard]] bool compare_exchange(shared_type& cmp_and_old, shared_type exch)
		{
			const uptr _old = reinterpret_cast<uptr>(cmp_and_old.m_ptr);
			const uptr _new = reinterpret_cast<uptr>(exch.m_ptr);

			if (exch.m_ptr)
			{
				exch.d().refs += c_ref_mask;
			}

			atomic_ptr old;

			const uptr _val = m_val.fetch_op([&](uptr& val)
			{
				if (val >> c_ref_size == _old)
				{
					// Set new value
					val = _new << c_ref_size;
				}
				else if (val)
				{
					// Reference previous value
					val++;
				}
			});

			if (_val >> c_ref_size == _old)
			{
				// Success (exch is consumed, cmp_and_old is unchanged)
				if (exch.m_ptr)
				{
					exch.m_ptr = nullptr;
				}

				// Cleanup
				old.m_val.raw() = _val;
				return true;
			}

			atomic_ptr old_exch;
			old_exch.m_val.raw() = reinterpret_cast<uptr>(std::exchange(exch.m_ptr, nullptr)) << c_ref_size;

			// Set to reset old cmp_and_old value
			old.m_val.raw() = (cmp_and_old.m_ptr << c_ref_size) | c_ref_mask;

			if (!_val)
			{
				return false;
			}

			// Set referenced pointer
			cmp_and_old.m_ptr = std::launder(reinterpret_cast<element_type*>(_val >> c_ref_size));
			cmp_and_old.d()->refs++;

			// Dereference if still the same pointer
			const auto [_, did_deref] = m_val.fetch_op([_val](uptr& val)
			{
				if (val >> c_ref_size == _val >> c_ref_size)
				{
					val--;
					return true;
				}

				return false;
			});

			if (!did_deref)
			{
				// Otherwise fix ref count (atomic_ptr has been overwritten)
				cmp_and_old.d()->refs--;
			}

			return false;
		}

		// Unoptimized
		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U> != same_ptr::no>>
		shared_type compare_and_swap(const shared_ptr<U>& cmp, shared_type exch)
		{
			if constexpr (is_same_ptr<T, U>() == same_ptr::maybe)
				ensure(is_same_ptr_test<T, U>(cmp.m_ptr));

			shared_type old = cmp;
			static_cast<void>(compare_exchange(old, std::move(exch)));
			return old;
		}

		// More lightweight than compare_exchange
		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U> != same_ptr::no>>
		bool compare_and_swap_test(const shared_ptr<U>& cmp, shared_type exch)
		{
			if constexpr (is_same_ptr<T, U>() == same_ptr::maybe)
				ensure(is_same_ptr_test<T, U>(cmp.m_ptr));

			const uptr _old = reinterpret_cast<uptr>(cmp.m_ptr);
			const uptr _new = reinterpret_cast<uptr>(exch.m_ptr);

			if (exch.m_ptr)
			{
				exch.d().refs += c_ref_mask;
			}

			atomic_ptr old;

			const auto [_val, ok] = m_val.fetch_op([&](uptr& val)
			{
				if (val >> c_ref_size == _old)
				{
					// Set new value
					val = _new << c_ref_size;
					return true;
				}

				return false;
			});

			if (ok)
			{
				// Success (exch is consumed, cmp_and_old is unchanged)
				exch.m_ptr = nullptr;
				old.m_val.raw() = _val;
				return true;
			}

			// Failure (return references)
			old.m_val.raw() = reinterpret_cast<uptr>(std::exchange(exch.m_ptr, nullptr)) << c_ref_size;
			return false;
		}

		// Unoptimized
		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U> != same_ptr::no>>
		shared_type compare_and_swap(const single_ptr<U>& cmp, shared_type exch)
		{
			if constexpr (is_same_ptr<T, U>() == same_ptr::maybe)
				ensure(is_same_ptr_test<T, U>(cmp.m_ptr));

			shared_type old = cmp;
			static_cast<void>(compare_exchange(old, std::move(exch)));
			return old;
		}

		// Supplementary
		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U> != same_ptr::no>>
		bool compare_and_swap_test(const single_ptr<U>& cmp, shared_type exch)
		{
			if constexpr (is_same_ptr<T, U>() == same_ptr::maybe)
				ensure(is_same_ptr_test<T, U>(cmp.m_ptr));

			return compare_and_swap_test(reinterpret_cast<const shared_ptr<U>&>(cmp), std::move(exch));
		}

		// Helper utility
		void push_head(shared_type& next, shared_type exch) noexcept
		{
			if (exch.m_ptr) [[likely]]
			{
				// Add missing references first
				exch.d()->refs += c_ref_mask;
			}

			if (next.m_ptr) [[unlikely]]
			{
				// Just in case
				next.reset();
			}

			atomic_ptr old;
			old.m_val.raw() = m_val.load();

			do
			{
				// Update old head with current value
				next.m_ptr = reinterpret_cast<T*>(old.m_val.raw() >> c_ref_size);

			} while (!m_val.compare_exchange(old.m_val.raw(), reinterpret_cast<uptr>(exch.m_ptr) << c_ref_size));

			// This argument is consumed (moved from)
			exch.m_ptr = nullptr;

			if (next.m_ptr)
			{
				// Compensation for `next` assignment
				old.m_val.raw() += 1;
			}
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

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U> != same_ptr::no>>
		bool is_equal(const shared_ptr<U>& r) const noexcept
		{
			return observe() == r.get();
		}

		template <typename U, typename = std::enable_if_t<is_same_ptr_cast_v<T, U> != same_ptr::no>>
		bool is_equal(const single_ptr<U>& r) const noexcept
		{
			return observe() == r.get();
		}

		template <atomic_wait::op Flags = atomic_wait::op::eq>
		void wait(const volatile void* value, atomic_wait_timeout timeout = atomic_wait_timeout::inf)
		{
			m_val.wait<Flags>(reinterpret_cast<uptr>(value) << c_ref_size, c_ptr_mask, timeout);
		}

		void notify_one()
		{
			m_val.notify_one(c_ptr_mask);
		}

		void notify_all()
		{
			m_val.notify_all(c_ptr_mask);
		}
	};

	// Some nullptr replacement for few cases
	constexpr struct null_ptr_t
	{
		template <typename T>
		constexpr operator single_ptr<T>() const noexcept
		{
			return {};
		}

		template <typename T>
		constexpr operator shared_ptr<T>() const noexcept
		{
			return {};
		}

		template <typename T>
		constexpr operator atomic_ptr<T>() const noexcept
		{
			return {};
		}

		explicit constexpr operator bool() const noexcept
		{
			return false;
		}

		constexpr operator std::nullptr_t() const noexcept
		{
			return nullptr;
		}

		constexpr std::nullptr_t get() const noexcept
		{
			return nullptr;
		}

	} null_ptr;
}

namespace atomic_wait
{
	template <typename T>
	constexpr u128 default_mask<stx::atomic_ptr<T>> = stx::c_ptr_mask;

	template <typename T>
	constexpr u128 get_value(stx::atomic_ptr<T>&, const volatile void* value = nullptr)
	{
		return reinterpret_cast<uptr>(value) << stx::c_ref_size;
	}
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

using stx::null_ptr;
using stx::single_ptr;
using stx::shared_ptr;
using stx::atomic_ptr;
using stx::make_single;
using stx::make_single_value;
using stx::make_shared_value;
