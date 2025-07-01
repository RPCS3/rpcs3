#pragma once // No BOM and only basic ASCII in this header, or a neko will die

#include <memory>
#include <utility>
#include "atomic.hpp"
#include "bless.hpp"

namespace stx
{
	template <typename To, typename From>
	constexpr bool same_ptr_implicit_v = std::is_convertible_v<const volatile From*, const volatile To*> ? PtrSame<From, To> : false;

	template <typename T>
	class single_ptr;

	template <typename T>
	class shared_ptr;

	template <typename T>
	class atomic_ptr;

	// Use 16 bits as atomic_ptr internal counter of borrowed refs
	constexpr uint c_ref_mask = 0xffff;

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
	class shared_data final : align_filler<sizeof(shared_counter), alignof(T)>
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
	class shared_data<T[]> final : align_filler<sizeof(shared_counter) + sizeof(usz), alignof(T)>
	{
	public:
		usz m_count{};

		shared_counter m_ctr{};

		constexpr shared_data() noexcept = default;
	};

	struct null_ptr_t;

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
		using element_type = std::remove_extent_t<T>;

		constexpr single_ptr() noexcept = default;

		single_ptr(const single_ptr&) = delete;

		// Default constructor or null_ptr should be used instead
		[[deprecated("Use null_ptr")]] single_ptr(std::nullptr_t) = delete;

		explicit single_ptr(shared_data<T>&, element_type* ptr) noexcept
			: m_ptr(ptr)
		{
		}

		single_ptr(single_ptr&& r) noexcept
			: m_ptr(r.m_ptr)
		{
			r.m_ptr = nullptr;
		}

		template <typename U> requires same_ptr_implicit_v<T, U>
		single_ptr(single_ptr<U>&& r) noexcept
		{
			m_ptr = r.m_ptr;
			r.m_ptr = nullptr;
		}

		~single_ptr() noexcept
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

		template <typename U> requires same_ptr_implicit_v<T, U>
		single_ptr& operator=(single_ptr<U>&& r) noexcept
		{
			single_ptr(std::move(r)).swap(*this);
			return *this;
		}

		void reset() noexcept
		{
			if (m_ptr) [[likely]]
			{
				const auto o = d();
				ensure(o->refs == 1);
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

		element_type& operator*() const noexcept requires (!std::is_void_v<element_type>)
		{
			return *m_ptr;
		}

		element_type* operator->() const noexcept
		{
			return m_ptr;
		}

		element_type& operator[](std::ptrdiff_t idx) const noexcept requires (!std::is_void_v<element_type> && std::is_array_v<T>)
		{
			return m_ptr[idx];
		}

		template <typename... Args> requires (std::is_invocable_v<T, Args&&...>)
		decltype(auto) operator()(Args&&... args) const noexcept
		{
			return std::invoke(*m_ptr, std::forward<Args>(args)...);
		}

		explicit constexpr operator bool() const noexcept
		{
			return m_ptr != nullptr;
		}

		// "Moving" "static cast"
		template <typename U> requires PtrSame<T, U>
		explicit operator single_ptr<U>() && noexcept
		{
			single_ptr<U> r;
			r.m_ptr = static_cast<decltype(r.m_ptr)>(std::exchange(m_ptr, nullptr));
			return r;
		}

		template <typename U> requires same_ptr_implicit_v<T, U>
		bool operator==(const single_ptr<U>& r) const noexcept
		{
			return get() == r.get();
		}
	};

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#elif !defined(_MSC_VER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif

	template <typename T, bool Init = true, typename... Args>
		requires(!std::is_unbounded_array_v<T> && (Init || std::is_array_v<T>) && (Init || !sizeof...(Args)))
	static single_ptr<T> make_single(Args&&... args) noexcept
	{
		static_assert(offsetof(shared_data<T>, m_data) - offsetof(shared_data<T>, m_ctr) == sizeof(shared_counter));

		using etype = std::remove_extent_t<T>;

		shared_data<T>* ptr = nullptr;

		if constexpr (!std::is_array_v<T>)
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

	template <typename T, bool Init = true, usz Align = alignof(std::remove_extent_t<T>)>
		requires (std::is_unbounded_array_v<T> && std::is_default_constructible_v<std::remove_extent_t<T>>)
	static single_ptr<T> make_single(usz count) noexcept
	{
		static_assert(sizeof(shared_data<T>) - offsetof(shared_data<T>, m_ctr) == sizeof(shared_counter));

		using etype = std::remove_extent_t<T>;

		const usz size = sizeof(shared_data<T>) + count * sizeof(etype);

		std::byte* bytes = nullptr;

		if constexpr (Align > (__STDCPP_DEFAULT_NEW_ALIGNMENT__))
		{
			bytes = static_cast<std::byte*>(::operator new(size, std::align_val_t{Align}));
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

			if constexpr (Align > (__STDCPP_DEFAULT_NEW_ALIGNMENT__))
			{
				::operator delete[](bytes, std::align_val_t{Align});
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

#ifdef __clang__
#pragma clang diagnostic pop
#elif !defined(_MSC_VER)
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
		template <typename Type>
		friend shared_ptr<Type> make_shared_from_this(const Type* _this) noexcept;

		template <typename U> requires same_ptr_implicit_v<T, U>
		shared_ptr(const shared_ptr<U>& r) noexcept
		{
			m_ptr = r.m_ptr;
			if (m_ptr)
				d()->refs++;
		}

		shared_ptr(shared_ptr&& r) noexcept
			: m_ptr(r.m_ptr)
		{
			r.m_ptr = nullptr;
		}

		template <typename U> requires same_ptr_implicit_v<T, U>
		shared_ptr(shared_ptr<U>&& r) noexcept
		{
			m_ptr = r.m_ptr;
			r.m_ptr = nullptr;
		}

		template <typename U> requires same_ptr_implicit_v<T, U>
		shared_ptr(single_ptr<U>&& r) noexcept
		{
			m_ptr = r.m_ptr;
			r.m_ptr = nullptr;
		}

		~shared_ptr() noexcept
		{
			reset();
		}

		shared_ptr& operator=(const shared_ptr& r) noexcept
		{
			shared_ptr(r).swap(*this);
			return *this;
		}

		[[deprecated("Use null_ptr")]] shared_ptr& operator=(std::nullptr_t) = delete;

		template <typename U> requires same_ptr_implicit_v<T, U>
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

		template <typename U> requires same_ptr_implicit_v<T, U>
		shared_ptr& operator=(shared_ptr<U>&& r) noexcept
		{
			shared_ptr(std::move(r)).swap(*this);
			return *this;
		}

		template <typename U> requires same_ptr_implicit_v<T, U>
		shared_ptr& operator=(single_ptr<U>&& r) noexcept
		{
			shared_ptr(std::move(r)).swap(*this);
			return *this;
		}

		// Set to null
		void reset() noexcept
		{
			if (m_ptr) [[unlikely]]
			{
				const auto o = d();

				if (!--o->refs)
				{
					o->destroy(o);
				}

				m_ptr = nullptr;
			}
		}

		// Converts to unique (single) ptr if reference is 1. Nullifies self on success.
		template <typename U> requires PtrSame<T, U>
		single_ptr<U> try_convert_to_single_ptr() noexcept
		{
			if (const auto o = m_ptr ? d() : nullptr; o && o->refs == 1u)
			{
				// Convert last reference to single_ptr instance.
				single_ptr<U> r;
				r.m_ptr = static_cast<decltype(r.m_ptr)>(std::exchange(m_ptr, nullptr));
				return r;
			}

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

		element_type& operator*() const noexcept requires (!std::is_void_v<element_type>)
		{
			return *m_ptr;
		}

		element_type* operator->() const noexcept
		{
			return m_ptr;
		}

		element_type& operator[](std::ptrdiff_t idx) const noexcept requires (!std::is_void_v<element_type> && std::is_array_v<T>)
		{
			return m_ptr[idx];
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

		explicit constexpr operator bool() const noexcept
		{
			return m_ptr != nullptr;
		}

		// Basic "static cast" support
		template <typename U> requires PtrSame<T, U>
		explicit operator shared_ptr<U>() const& noexcept
		{
			if (m_ptr)
			{
				d()->refs++;
			}

			shared_ptr<U> r;
			r.m_ptr = static_cast<decltype(r.m_ptr)>(m_ptr);
			return r;
		}

		// "Moving" "static cast"
		template <typename U> requires PtrSame<T, U>
		explicit operator shared_ptr<U>() && noexcept
		{
			shared_ptr<U> r;
			r.m_ptr = static_cast<decltype(r.m_ptr)>(std::exchange(m_ptr, nullptr));
			return r;
		}

		template <typename U> requires same_ptr_implicit_v<T, U>
		bool operator==(const shared_ptr<U>& r) const noexcept
		{
			return get() == r.get();
		}
	};

	template <typename T, typename... Args>
		requires(!std::is_unbounded_array_v<T> && std::is_constructible_v<std::remove_extent_t<T>, Args&& ...>)
	static shared_ptr<T> make_shared(Args&&... args) noexcept
	{
		return make_single<T>(std::forward<Args>(args)...);
	}

	template <typename T, bool Init = true>
		requires (std::is_unbounded_array_v<T> && std::is_default_constructible_v<std::remove_extent_t<T>>)
	static shared_ptr<T> make_shared(usz count) noexcept
	{
		return make_single<T, Init>(count);
	}

	template <typename T>
		requires (std::is_constructible_v<std::remove_reference_t<T>, T&&>)
	static shared_ptr<std::remove_reference_t<T>> make_shared_value(T&& value) noexcept
	{
		return make_single_value(std::forward<T>(value));
	}

	// Not-so-aliasing constructor: emulates std::enable_shared_from_this without its overhead
	template <typename T>
	static shared_ptr<T> make_shared_from_this(const T* _this) noexcept
	{
		shared_ptr<T> r;
		r.m_ptr = const_cast<T*>(_this);

		if (!_this) [[unlikely]]
		{
			return r;
		}

		// Random checks which may fail on invalid pointer
		ensure((r.d()->refs++ - 1) >> 58 == 0);
		return r;
	}

	// Atomic simplified shared pointer
	template <typename T>
	class atomic_ptr
	{
	private:
		struct fat_ptr
		{
			uptr ptr{};
			u32 is_non_null{};
			u32 ref_ctr{};
		};

		mutable atomic_t<fat_ptr> m_val{fat_ptr{}};

		static shared_counter* d(fat_ptr val) noexcept
		{
			return std::launder(reinterpret_cast<shared_counter*>(val.ptr - sizeof(shared_counter)));
		}

		shared_counter* d() const noexcept
		{
			return d(m_val);
		}

		static fat_ptr to_val(const volatile std::remove_extent_t<T>* ptr) noexcept
		{
			return fat_ptr{reinterpret_cast<uptr>(ptr), ptr != nullptr, 0};
		}

		static fat_ptr to_val(uptr ptr) noexcept
		{
			return fat_ptr{ptr, ptr != 0, 0};
		}

		static std::remove_extent_t<T>* ptr_to(fat_ptr val) noexcept
		{
			return reinterpret_cast<std::remove_extent_t<T>*>(val.ptr);
		}

		template <typename U>
		friend class atomic_ptr;

		// Helper struct to check if a type is an instance of a template
		template <typename T1, template <typename> class Template>
		struct is_instance_of : std::false_type {};

		template <typename T1, template <typename> class Template>
		struct is_instance_of<Template<T1>, Template> : std::true_type {};

		template <typename T1>
		static constexpr bool is_stx_pointer = false
			|| is_instance_of<std::remove_cvref_t<T1>, shared_ptr>::value
			|| is_instance_of<std::remove_cvref_t<T1>, single_ptr>::value
			|| is_instance_of<std::remove_cvref_t<T1>, atomic_ptr>::value
			|| std::is_same_v<std::remove_cvref_t<T1>, null_ptr_t>;

	public:
		using element_type = std::remove_extent_t<T>;

		using shared_type = shared_ptr<T>;

		constexpr atomic_ptr() noexcept = default;

		// Optimized value construct
		template <typename... Args> requires (true
			&& sizeof...(Args) != 0
			&& !(sizeof...(Args) == 1 && (is_stx_pointer<Args> || ...))
			&& std::is_constructible_v<element_type, Args&&...>)
		explicit atomic_ptr(Args&&... args) noexcept
		{
			shared_type r = make_single<T>(std::forward<Args>(args)...);
			m_val.raw() = to_val(std::exchange(r.m_ptr, nullptr));
			d()->refs.raw() += c_ref_mask;
		}

		template <typename U> requires same_ptr_implicit_v<T, U>
		atomic_ptr(const shared_ptr<U>& r) noexcept
		{
			// Obtain a ref + as many refs as an atomic_ptr can additionally reference
			if (fat_ptr rval = to_val(r.m_ptr); rval.ptr != 0)
			{
				m_val.raw() = rval;
				d(rval)->refs += c_ref_mask + 1;
			}
		}

		template <typename U> requires same_ptr_implicit_v<T, U>
		atomic_ptr(shared_ptr<U>&& r) noexcept
		{
			if (fat_ptr rval = to_val(r.m_ptr); rval.ptr != 0)
			{
				m_val.raw() = rval;
				d(rval)->refs += c_ref_mask;
			}

			r.m_ptr = nullptr;
		}

		template <typename U> requires same_ptr_implicit_v<T, U>
		atomic_ptr(single_ptr<U>&& r) noexcept
		{
			if (fat_ptr rval = to_val(r.m_ptr); rval.ptr != 0)
			{
				m_val.raw() = rval;
				d(rval)->refs += c_ref_mask;
			}

			r.m_ptr = nullptr;
		}

		~atomic_ptr() noexcept
		{
			const fat_ptr v = m_val.raw();

			if (v.ptr)
			{
				const auto o = d(v);

				if (!o->refs.sub_fetch(c_ref_mask + 1 - (v.ref_ctr & c_ref_mask)))
				{
					o->destroy.load()(o);
				}
			}
		}

		// Optimized value assignment
		atomic_ptr& operator=(std::remove_cv_t<T> value) noexcept requires (!is_stx_pointer<T>)
		{
			shared_type r = make_single<T>(std::move(value));
			r.d()->refs.raw() += c_ref_mask;

			atomic_ptr old;
			old.m_val.raw() = m_val.exchange(to_val(std::exchange(r.m_ptr, nullptr)));
			return *this;
		}

		template <typename U> requires same_ptr_implicit_v<T, U>
		atomic_ptr& operator=(const shared_ptr<U>& r) noexcept
		{
			store(r);
			return *this;
		}

		template <typename U> requires same_ptr_implicit_v<T, U>
		atomic_ptr& operator=(shared_ptr<U>&& r) noexcept
		{
			store(std::move(r));
			return *this;
		}

		template <typename U> requires same_ptr_implicit_v<T, U>
		atomic_ptr& operator=(single_ptr<U>&& r) noexcept
		{
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
			const auto [prev, did_ref] = m_val.fetch_op([](fat_ptr& val)
			{
				if (val.ptr)
				{
					val.ref_ctr++;
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
			r.m_ptr = std::launder(ptr_to(prev));
			r.d()->refs++;

			// Dereference if still the same pointer
			const auto [_, did_deref] = m_val.fetch_op([prev = prev](fat_ptr& val)
			{
				if (val.ptr == prev.ptr)
				{
					val.ref_ctr--;
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
			const auto [prev, did_ref] = m_val.fetch_op([](fat_ptr& val)
			{
				if (val.ptr)
				{
					val.ref_ctr++;
					return true;
				}

				return false;
			});

			// Set fake unreferenced pointer
			if (did_ref)
			{
				r.m_ptr = std::launder(ptr_to(prev));
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
			const auto [_, did_deref] = m_val.fetch_op([prev = prev](fat_ptr& val)
			{
				if (val.ptr == prev.ptr)
				{
					val.ref_ctr--;
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

		// Create an object from variadic args
		// If a type needs shared_type to be constructed, std::reference_wrapper can be used
		template <typename... Args> requires (true
			&& sizeof...(Args) != 0
			&& !(sizeof...(Args) == 1 && (is_stx_pointer<Args> || ...))
			&& std::is_constructible_v<element_type, Args&&...>)
		void store(Args&&... args) noexcept
		{
			shared_type r = make_single<T>(std::forward<Args>(args)...);
			r.d()->refs.raw() += c_ref_mask;

			atomic_ptr old;
			old.m_val.raw() = m_val.exchange(to_val(std::exchange(r.m_ptr, nullptr)));
		}

		void store(shared_type value) noexcept
		{
			if (value.m_ptr)
			{
				// Consume value and add refs
				value.d()->refs += c_ref_mask;
			}

			atomic_ptr old;
			old.m_val.raw() = m_val.exchange(to_val(std::exchange(value.m_ptr, nullptr)));
		}

		template <typename... Args> requires (true
			&& sizeof...(Args) != 0
			&& !(sizeof...(Args) == 1 && (is_stx_pointer<Args> || ...))
			&& std::is_constructible_v<element_type, Args&...>)
		[[nodiscard]] shared_type exchange(Args&&... args) noexcept
		{
			shared_type r = make_single<T>(std::forward<Args>(args)...);
			r.d()->refs.raw() += c_ref_mask;

			atomic_ptr old;
			old.m_val.raw() = m_val.exchange(to_val(r.m_ptr));
			old.m_val.raw().ref_ctr += 1;

			r.m_ptr = std::launder(ptr_to(old.m_val));
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
			old.m_val.raw() = m_val.exchange(to_val(value.m_ptr));
			old.m_val.raw().ref_ctr += 1;

			value.m_ptr = std::launder(ptr_to(old.m_val));
			return value;
		}

		// Ineffective
		[[nodiscard]] bool compare_exchange(shared_type& cmp_and_old, shared_type exch)
		{
			const uptr _old = reinterpret_cast<uptr>(cmp_and_old.m_ptr);
			const uptr _new = reinterpret_cast<uptr>(exch.m_ptr);

			if (exch.m_ptr)
			{
				exch.d()->refs += c_ref_mask;
			}

			atomic_ptr old;

			const fat_ptr _val = m_val.fetch_op([&](fat_ptr& val)
			{
				if (val.ptr == _old)
				{
					// Set new value
					val = to_val(_new);
				}
				else if (val.ptr != 0)
				{
					// Reference previous value
					val.ref_ctr++;
				}
			});

			if (_val.ptr == _old)
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
			old_exch.m_val.raw() = to_val(std::exchange(exch.m_ptr, nullptr));

			// Set to reset old cmp_and_old value
			old.m_val.raw() = to_val(cmp_and_old.m_ptr);
			old.m_val.raw().ref_ctr |= c_ref_mask;

			if (!_val.ptr)
			{
				return false;
			}

			// Set referenced pointer
			cmp_and_old.m_ptr = std::launder(ptr_to(_val));
			cmp_and_old.d()->refs++;

			// Dereference if still the same pointer
			const auto [_, did_deref] = m_val.fetch_op([_val](fat_ptr& val)
			{
				if (val.ptr == _val.ptr)
				{
					val.ref_ctr--;
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
		template <typename U> requires same_ptr_implicit_v<T, U>
		shared_type compare_and_swap(const shared_ptr<U>& cmp, shared_type exch)
		{
			shared_type old = cmp;
			static_cast<void>(compare_exchange(old, std::move(exch)));
			return old;
		}

		// More lightweight than compare_exchange
		template <typename U> requires same_ptr_implicit_v<T, U>
		bool compare_and_swap_test(const shared_ptr<U>& cmp, shared_type exch)
		{
			const uptr _old = reinterpret_cast<uptr>(cmp.m_ptr);
			const uptr _new = reinterpret_cast<uptr>(exch.m_ptr);

			if (exch.m_ptr)
			{
				exch.d()->refs += c_ref_mask;
			}

			atomic_ptr old;

			const auto [_val, ok] = m_val.fetch_op([&](fat_ptr& val)
			{
				if (val.ptr == _old)
				{
					// Set new value
					val = to_val(_new);
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
			old.m_val.raw() = to_val(std::exchange(exch.m_ptr, nullptr));
			return false;
		}

		// Unoptimized
		template <typename U> requires same_ptr_implicit_v<T, U>
		shared_type compare_and_swap(const single_ptr<U>& cmp, shared_type exch)
		{
			shared_type old = cmp;
			static_cast<void>(compare_exchange(old, std::move(exch)));
			return old;
		}

		// Supplementary
		template <typename U> requires same_ptr_implicit_v<T, U>
		bool compare_and_swap_test(const single_ptr<U>& cmp, shared_type exch)
		{
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
				next.m_ptr = std::launder(ptr_to(old.m_val.raw()));

			} while (!m_val.compare_exchange(old.m_val.raw(), to_val(exch.m_ptr)));

			// This argument is consumed (moved from)
			exch.m_ptr = nullptr;

			if (next.m_ptr)
			{
				// Compensation for `next` assignment
				old.m_val.raw().ref_ctr += 1;
			}
		}

		// Simple atomic load is much more effective than load(), but it's a non-owning reference
		T* observe() const noexcept
		{
			return std::launder(ptr_to(m_val));
		}

		explicit constexpr operator bool() const noexcept
		{
			return observe() != nullptr;
		}

		template <typename U> requires same_ptr_implicit_v<T, U>
		bool is_equal(const shared_ptr<U>& r) const noexcept
		{
			return static_cast<volatile const void*>(observe()) == r.get();
		}

		template <typename U> requires same_ptr_implicit_v<T, U>
		bool is_equal(const single_ptr<U>& r) const noexcept
		{
			return static_cast<volatile const void*>(observe()) == r.get();
		}

		atomic_t<u32> &get_wait_atomic()
		{
			return *utils::bless<atomic_t<u32>>(&m_val.raw().is_non_null);
		}

		void wait(std::nullptr_t, atomic_wait_timeout timeout = atomic_wait_timeout::inf)
		{
			get_wait_atomic().wait(0, timeout);
		}

		void notify_one()
		{
			get_wait_atomic().notify_one();
		}

		void notify_all()
		{
			get_wait_atomic().notify_all();
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

	} null_ptr;
}

template <typename T>
struct std::hash<stx::single_ptr<T>>
{
	usz operator()(const stx::single_ptr<T>& x) const noexcept
	{
		return std::hash<T*>()(x.get());
	}
};

template <typename T>
struct std::hash<stx::shared_ptr<T>>
{
	usz operator()(const stx::shared_ptr<T>& x) const noexcept
	{
		return std::hash<T*>()(x.get());
	}
};

using stx::null_ptr;
using stx::single_ptr;
using stx::shared_ptr;
using stx::atomic_ptr;
using stx::make_single;
using stx::make_shared;
using stx::make_single_value;
using stx::make_shared_value;
