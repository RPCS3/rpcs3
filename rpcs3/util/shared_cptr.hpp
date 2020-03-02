#pragma once

#include <utility>
#include <type_traits>
#include <cstddef>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace stx
{
	template <typename T>
	class shared_data;

	template <typename T>
	class unique_data;

	// Common internal layout
	class atomic_base
	{
	public:
	#if defined(__x86_64__) || defined(_M_X64)
		using ptr_type = long long;

		static const long long c_ref_init = 0x10000;
		static const ptr_type c_ref_mask = 0xffff;
		static const ptr_type c_ptr_mask = ~0;
		static const ptr_type c_ptr_shift = 16;
		static const auto c_ptr_align = alignof(long long);
	#else
		using ptr_type = unsigned long long;

		static const long long c_ref_init = 0x10;
		static const ptr_type c_ref_mask = 0xf;
		static const ptr_type c_ptr_mask = ~c_ref_mask;
		static const ptr_type c_ptr_shift = 0;
		static const auto c_ptr_align = 16;
	#endif

	protected:
		// Combined borrowed refcounter and object pointer
		mutable ptr_type m_val;

		template <typename T, bool Const>
		friend class atomic_cptr;

		constexpr atomic_base() noexcept
			: m_val(0)
		{
		}

		explicit constexpr atomic_base(ptr_type val) noexcept
			: m_val(val)
		{
		}

		template <typename T>
		explicit atomic_base(shared_data<T>* ptr) noexcept
			: m_val(reinterpret_cast<ptr_type>(ptr) << c_ptr_shift)
		{
			if (ptr)
			{
				// Fixup reference counter
				m_val |= (ptr->m_ref_count - 1) & c_ref_mask;
			}
		}

		template <typename T>
		shared_data<T>* ptr_get() const noexcept
		{
			return reinterpret_cast<shared_data<T>*>(val_load() >> c_ptr_shift & c_ptr_mask);
		}

		ptr_type val_load() const noexcept
		{
	#ifdef _MSC_VER
			return const_cast<const volatile ptr_type&>(m_val);
	#else
			return __atomic_load_n(&m_val, __ATOMIC_SEQ_CST);
	#endif
		}

		ptr_type val_exchange(ptr_type val) noexcept
		{
	#ifdef _MSC_VER
			return _InterlockedExchange64(&m_val, val);
	#else
			return __atomic_exchange_n(&m_val, val, __ATOMIC_SEQ_CST);
	#endif
		}

		bool val_compare_exchange(ptr_type& expected, ptr_type val) const noexcept
		{
	#ifdef _MSC_VER
			ptr_type x = expected;
			expected = _InterlockedCompareExchange64(&m_val, val, x);
			return x == expected;
	#else
			return __atomic_compare_exchange_n(&m_val, &expected, val, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	#endif
		}

		// Load, acquiring half of the references from the pointer
		ptr_type ref_halve() const noexcept;

		// Load, actively acquiring a reference from the pointer
		ptr_type ref_load() const noexcept;

		// Return acquired reference if applicable
		void ref_fix(ptr_type& old) const noexcept;
	};

	// Control block with data and reference counter
	template <typename T>
	class alignas(atomic_base::c_ptr_align) shared_data final
	{
	public:
		// Immutable data
		T m_data;

		// Main reference counter
		long long m_ref_count = atomic_base::c_ref_init;

		template <typename... Args>
		explicit constexpr shared_data(Args&&... args) noexcept
			: m_data(std::forward<Args>(args)...)
		{
			static_assert(offsetof(shared_data, m_data) == 0);
		}

		// Atomic access to the ref counter
		long long fetch_add(long long value)
		{
	#ifdef _MSC_VER
			return _InterlockedExchangeAdd64(&m_ref_count, value);
	#else
			return __atomic_fetch_add(&m_ref_count, value, __ATOMIC_SEQ_CST);
	#endif
		}
	};

	// Unique ownership pointer to mutable data, suitable for conversion to shared_cptr
	template <typename T>
	class unique_data : protected atomic_base
	{
		using cb = atomic_base;

	public:
		constexpr unique_data() noexcept
			: atomic_base()
		{
		}

		explicit unique_data(shared_data<T>* data) noexcept
			: atomic_base(data)
		{
		}

		unique_data(const unique_data&) = delete;

		unique_data(unique_data&& r) noexcept
			: atomic_base(r.m_val)
		{
			r.m_val = 0;
		}

		unique_data& operator=(const unique_data&) = delete;

		unique_data& operator=(unique_data&& r) noexcept
		{
			unique_data(std::move(r)).swap(*this);
			return *this;
		}

		~unique_data()
		{
			reset();
		}

		void reset() noexcept
		{
			delete get();
			this->m_val = 0;
		}

		void swap(unique_data& r) noexcept
		{
			std::swap(this->m_val, r.m_val);
		}

		[[nodiscard]] shared_data<T>* release() noexcept
		{
			auto result = this->ptr_get<T>();
			this->m_val = 0;
			return result;
		}

		T* get() const noexcept
		{
			return &this->ptr_get<T>()->m_data;
		}

		T& operator*() const noexcept
		{
			return *get();
		}

		T* operator->() const noexcept
		{
			return get();
		}

		explicit operator bool() const noexcept
		{
			return this->m_val != 0;
		}

		template <typename... Args>
		static unique_data make(Args&&... args) noexcept
		{
			return unique_data(new shared_data<T>(std::forward<Args>(args)...));
		}
	};

	// Shared pointer to immutable data
	template <typename T, bool Const = true>
	class shared_cptr : protected atomic_base
	{
		using cb = atomic_base;

	protected:
		using atomic_base::m_val;

	public:
		constexpr shared_cptr() noexcept
			: atomic_base()
		{
		}

		explicit shared_cptr(shared_data<T>* data) noexcept
			: atomic_base(data)
		{
		}

		shared_cptr(const shared_cptr& r) noexcept
			: atomic_base()
		{
			if (const auto old_val = r.val_load())
			{
				// Try to take references from the former pointer first
				if (const auto new_val = r.ref_halve())
				{
					this->m_val = new_val;
				}
				else
				{
					// If it fails, fallback to the control block and take max amount
					this->m_val = old_val | cb::c_ref_mask;
					this->ptr_get<T>()->fetch_add(cb::c_ref_init);
				}
			}
		}

		shared_cptr(shared_cptr&& r) noexcept
			: atomic_base(r.m_val)
		{
			r.m_val = 0;
		}

		shared_cptr(unique_data<T> r) noexcept
			: atomic_base(r.m_val)
		{
			r.m_val = 0;
		}

		shared_cptr& operator=(const shared_cptr& r) noexcept
		{
			shared_cptr(r).swap(*this);
			return *this;
		}

		shared_cptr& operator=(shared_cptr&& r) noexcept
		{
			shared_cptr(std::move(r)).swap(*this);
			return *this;
		}

		~shared_cptr()
		{
			reset();
		}

		// Set to null
		void reset() noexcept
		{
			if (const auto pdata = this->ptr_get<T>())
			{
				// Remove references
				const auto count = (cb::c_ref_mask & this->m_val) + 1;

				this->m_val = 0;

				if (pdata->fetch_add(-count) == count)
				{
					// Destroy if reference count became zero
					delete pdata;
				}
			}
		}

		// Possibly return reference(s) to specified shared pointer instance
		void reset_hint(const shared_cptr& r) noexcept
		{
			// TODO
			reset();
		}

		// Set to null, possibly returning a unique instance of shared data
		unique_data<T> release_unique() noexcept
		{
			if (const auto pdata = this->ptr_get<T>())
			{
				// Remove references
				const auto count = (cb::c_ref_mask & this->m_val) + 1;

				this->m_val = 0;

				if (pdata->fetch_add(-count) == count)
				{
					// Return data if reference count became zero
					pdata->m_ref_count = cb::c_ref_init;
					return unique_data<T>(pdata);
				}
			}

			return {};
		}

		void swap(shared_cptr& r) noexcept
		{
			std::swap(this->m_val, r.m_val);
		}

		std::conditional_t<Const, const T*, T*> get() const noexcept
		{
			return &this->ptr_get<T>()->m_data;
		}

		std::conditional_t<Const, const T&, T&> operator*() const noexcept
		{
			return *get();
		}

		std::conditional_t<Const, const T*, T*> operator->() const noexcept
		{
			return get();
		}

		explicit operator bool() const noexcept
		{
			return this->val_load() != 0;
		}

		bool operator ==(const shared_cptr& rhs) const noexcept
		{
			return get() == rhs.get();
		}

		bool operator !=(const shared_cptr& rhs) const noexcept
		{
			return get() != rhs.get();
		}

		template <typename... Args>
		static shared_cptr make(Args&&... args) noexcept
		{
			return shared_cptr(new shared_data<T>(std::forward<Args>(args)...));
		}
	};

	template <typename T>
	using shared_mptr = shared_cptr<T, false>;

	// Atomic shared pointer to immutable data
	template <typename T, bool Const = true>
	class atomic_cptr : shared_cptr<T, Const>
	{
		using cb = atomic_base;
		using base = shared_cptr<T, Const>;

	public:
		constexpr atomic_cptr() noexcept
			: base()
		{
		}

		atomic_cptr(base value)
			: base(std::move(value))
		{
			if (const auto diff = cb::c_ref_mask - (this->m_val & cb::c_ref_mask); this->m_val && diff)
			{
				// Obtain max amount of references
				this->template ptr_get<T>()->fetch_add(diff);
				this->m_val |= cb::c_ref_mask;
			}
		}

		atomic_cptr(const atomic_cptr&) = delete;

		atomic_cptr& operator=(const atomic_cptr&) = delete;

		atomic_cptr& operator=(base value) noexcept
		{
			exchange(std::move(value));
			return *this;
		}

		void store(base value) noexcept
		{
			exchange(std::move(value));
		}

		base load() const noexcept
		{
			base result;
			static_cast<cb&>(result).m_val = this->ref_load();

			if (result)
			{
				// TODO: obtain max-1 and try to return as much as possible
				this->template ptr_get<T>()->fetch_add(1);
				this->ref_fix(static_cast<cb&>(result).m_val);
			}

			return result;
		}

		operator base() const noexcept
		{
			return load();
		}

		base exchange(base value) noexcept
		{
			static_cast<cb&>(value).m_val = this->val_exchange(static_cast<cb&>(value).m_val);
			return value;
		}

		// Simple atomic load is much more effective than load(), but it's a non-owning reference
		const void* observe() const noexcept
		{
			return this->get();
		}

		explicit operator bool() const noexcept
		{
			return observe() != nullptr;
		}

		bool is_equal(const base& r) const noexcept
		{
			return observe() == r.get();
		}

		// bool compare_and_swap_test_weak(const base& expected, base value) noexcept
		// {

		// }

		// bool compare_and_swap_test(const base& expected, base value) noexcept
		// {

		// }

		// bool compare_exchange_weak(base& expected, base value) noexcept
		// {
		// 	// TODO
		// }

		// bool compare_exchange(base& expected, base value) noexcept
		// {
		// 	// TODO
		// }

		// void atomic_op();

		using base::make;
	};

	template <typename T>
	using atomic_mptr = atomic_cptr<T, false>;
}
