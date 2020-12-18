#pragma once

#include <cstdint>
#include "util/atomic.hpp"

namespace stx
{
	// Unsigned 64-bit atomic for multi-cas (occupies 128 bits)
	class alignas(16) atomic2
	{
		// First 64-bit value is an actual value, second one is an allocated control block pointer (if not zero)
		s64 m_data[2]{};

		friend class multi_cas_record;

	public:
		// Can't be really uninitialized or it'll be fundamentally broken
		constexpr atomic2() noexcept = default;

		atomic2(const atomic2&) = delete;

		atomic2& operator=(const atomic2&) = delete;

		constexpr atomic2(u64 value) noexcept
			: m_data{static_cast<s64>(value), s64{0}}
		{
		}

		// Simply observe the state
		u64 load() const noexcept
		{
			return atomic_storage<u64>::load(m_data[0]);
		}

		// void wait(u64 old_value) const noexcept;
		// void notify_one() noexcept;
		// void notify_all() noexcept;
	};

	// Atomic CAS item
	class multi_cas_item
	{
		atomic2* m_addr;
		u64 m_old;
		u64 m_new;

		friend class multi_cas_record;

	public:
		multi_cas_item() noexcept = default;

		multi_cas_item(const multi_cas_item&) = delete;

		multi_cas_item& operator=(const multi_cas_item&) = delete;

		u64 get_old() const noexcept
		{
			return m_old;
		}

		operator u64() const noexcept
		{
			return m_new;
		}

		void operator=(u64 value) noexcept
		{
			m_new = value;
		}
	};

	// An object passed to multi_cas lambda
	class alignas(64) multi_cas_record
	{
		// Ref counter and Multi-CAS state
		atomic_t<u64> m_state;

		// Total number of CASes
		u64 m_count;

		// Support up to 10 CASes
		multi_cas_item m_list[10];

	public:
		// Read atomic value and allocate "writable" item
		multi_cas_item& load(atomic2& atom) noexcept
		{
			if (m_count >= std::size(m_list))
			{
				std::abort();
			}

			auto& r  = m_list[m_count++];
			r.m_addr = &atom;
			r.m_old  = atom.load();
			r.m_new  = r.m_old;
			return r;
		}

		// Reset transaction (invalidates item references)
		void cancel() noexcept
		{
			m_count = 0;
		}

		// Try to commit sudoku (don't call)
		bool commit() const noexcept;
	};

	template <typename T>
	struct multi_cas_result
	{
		static constexpr bool is_void = false;

		T ret;
	};

	template <>
	struct multi_cas_result<void>
	{
		static constexpr bool is_void = true;
	};

	template <typename Context>
	class multi_cas final : Context, multi_cas_record, public multi_cas_result<std::invoke_result_t<Context, multi_cas_record&>>
	{
		using result = multi_cas_result<std::invoke_result_t<Context, multi_cas_record&>>;
		using record = multi_cas_record;

	public:
		// Implicit deduction guide candidate constructor (for lambda)
		multi_cas(Context&& f) noexcept
			: Context(std::forward<Context>(f))
		{
			while (true)
			{
				multi_cas_record& rec = *this;
				record::cancel();

				if constexpr (result::is_void)
				{
					Context::operator()(rec);
				}
				else
				{
					result::ret = Context::operator()(rec);
				}

				if (record::commit())
				{
					return;
				}
			}
		}
	};
}