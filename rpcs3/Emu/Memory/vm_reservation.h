#pragma once

#include "vm.h"
#include "vm_locking.h"
#include "util/atomic.hpp"
#include "util/tsc.hpp"
#include <functional>

#ifdef _MSC_VER
extern "C"
{
	u32 _xbegin();
	void _xend();
}
#endif

namespace vm
{
	enum : u64
	{
		rsrv_lock_mask = 127,
		rsrv_unique_lock = 64,
		rsrv_putunc_flag = 32,
	};

	// Get reservation status for further atomic update: last update timestamp
	inline atomic_t<u64>& reservation_acquire(u32 addr)
	{
		// Access reservation info: stamp and the lock bit
		return *reinterpret_cast<atomic_t<u64>*>(g_reservations + (addr & 0xff80) / 2);
	}

	// Update reservation status
	void reservation_update(u32 addr);
	std::pair<bool, u64> try_reservation_update(u32 addr);

	struct reservation_waiter_t
	{
		u32 wait_flag = 0;
		u32 waiters_count = 0;
	};

	static inline atomic_t<reservation_waiter_t>* reservation_notifier(u32 raddr, u64 rtime)
	{
		constexpr u32 wait_vars_for_each = 64;
		constexpr u32 unique_address_bit_mask = 0b1111;
		constexpr u32 unique_rtime_bit_mask = 0b1;

		extern std::array<atomic_t<reservation_waiter_t>, wait_vars_for_each * (unique_address_bit_mask + 1) * (unique_rtime_bit_mask + 1)> g_resrv_waiters_count;

		// Storage efficient method to distinguish different nearby addresses (which are likely)
		const usz index = std::popcount(raddr & -2048) * (1 << 5) + ((rtime / 128) & unique_rtime_bit_mask) * (1 << 4) + ((raddr / 128) & unique_address_bit_mask);
		return &g_resrv_waiters_count[index];
	}

	// Returns waiter count
	static inline u32 reservation_notifier_count(u32 raddr, u64 rtime)
	{
		return reservation_notifier(raddr, rtime)->load().waiters_count;
	}

	static inline void reservation_notifier_end_wait(atomic_t<reservation_waiter_t>& waiter)
	{
		waiter.atomic_op([](reservation_waiter_t& value)
		{
			if (value.waiters_count == 1 && value.wait_flag % 2 == 1)
			{
				// Make wait_flag even (disabling notification on last waiter)
				value.wait_flag++;
			}

			value.waiters_count--;
		});
	}

	static inline std::pair<atomic_t<reservation_waiter_t>*, u32> reservation_notifier_begin_wait(u32 raddr, u64 rtime)
	{
		atomic_t<reservation_waiter_t>& waiter = *reservation_notifier(raddr, rtime);

		u32 wait_flag = 0;

		waiter.atomic_op([&](reservation_waiter_t& value)
		{
			if (value.wait_flag % 2 == 0)
			{
				// Make wait_flag odd (for notification deduplication detection)
				value.wait_flag++;
			}

			wait_flag = value.wait_flag;
			value.waiters_count++;
		});

		if ((reservation_acquire(raddr) & -128) != rtime)
		{
			reservation_notifier_end_wait(waiter);
			return {};
		}

		return { &waiter, wait_flag };
	}

	atomic_t<u32>* reservation_notifier_notify(u32 raddr, u64 rtime, bool postpone = false);

	u64 reservation_lock_internal(u32, atomic_t<u64>&);

	void reservation_shared_lock_internal(atomic_t<u64>&);

	inline bool reservation_try_lock(atomic_t<u64>& res, u64 rtime)
	{
		if (res.compare_and_swap_test(rtime, rtime | rsrv_unique_lock)) [[likely]]
		{
			return true;
		}

		return false;
	}

	inline std::pair<atomic_t<u64>&, u64> reservation_lock(u32 addr)
	{
		auto res = &vm::reservation_acquire(addr);
		auto rtime = res->load();

		if (rtime & 127 || !reservation_try_lock(*res, rtime)) [[unlikely]]
		{
			static atomic_t<u64> no_lock{};

			rtime = reservation_lock_internal(addr, *res);

			if (rtime == umax)
			{
				res = &no_lock;
			}
		}

		return {*res, rtime};
	}

	// TODO: remove and make it external
	void reservation_op_internal(u32 addr, std::function<bool()> func);

	template <bool Ack = false, typename CPU, typename T, typename AT = u32, typename F>
	inline SAFE_BUFFERS(auto) reservation_op(CPU& /*cpu*/, _ptr_base<T, AT> ptr, F op)
	{
		// Atomic operation will be performed on aligned 128 bytes of data, so the data size and alignment must comply
		static_assert(sizeof(T) <= 128 && alignof(T) == sizeof(T), "vm::reservation_op: unsupported type");
		static_assert(std::is_trivially_copyable_v<T>, "vm::reservation_op: not triv copyable (optimization)");

		// Use "super" pointer to prevent access violation handling during atomic op
		const auto sptr = vm::get_super_ptr<T>(static_cast<u32>(ptr.addr()));

		// Prefetch some data
		//_m_prefetchw(sptr);
		//_m_prefetchw(reinterpret_cast<char*>(sptr) + 64);

		// Use 128-byte aligned addr
		const u32 addr = static_cast<u32>(ptr.addr()) & -128;

		auto& res = vm::reservation_acquire(addr);
		//_m_prefetchw(&res);

		// Lock reservation and perform heavyweight lock
		reservation_shared_lock_internal(res);

		u64 old_time = umax;

		if constexpr (std::is_void_v<std::invoke_result_t<F, T&>>)
		{
			{
				vm::writer_lock lock(addr);
				std::invoke(op, *sptr);
				old_time = res.fetch_add(127);
			}

			if constexpr (Ack)
				reservation_notifier_notify(addr, old_time);
			return;
		}
		else
		{
			auto result = std::invoke_result_t<F, T&>();
			{
				vm::writer_lock lock(addr);

				if ((result = std::invoke(op, *sptr)))
				{
					old_time = res.fetch_add(127);
				}
				else
				{
					old_time = res.fetch_sub(1);
				}
			}

			if (Ack && result)
				reservation_notifier_notify(addr, old_time);
			return result;
		}
	}

	// For internal usage
	[[noreturn]] void reservation_escape_internal();

	// Read memory value in pseudo-atomic manner
	template <typename CPU, typename T, typename AT = u32, typename F>
	inline SAFE_BUFFERS(auto) peek_op(CPU&& cpu, _ptr_base<T, AT> ptr, F op)
	{
		// Atomic operation will be performed on aligned 128 bytes of data, so the data size and alignment must comply
		static_assert(sizeof(T) <= 128 && alignof(T) == sizeof(T), "vm::peek_op: unsupported type");

		// Use 128-byte aligned addr
		const u32 addr = static_cast<u32>(ptr.addr()) & -128;

		while (true)
		{
			if constexpr (std::is_class_v<std::remove_cvref_t<CPU>>)
			{
				if (cpu.test_stopped())
				{
					reservation_escape_internal();
				}
			}

			const u64 rtime = vm::reservation_acquire(addr);

			if (rtime & 127)
			{
				continue;
			}

			// Observe data non-atomically and make sure no reservation updates were made
			if constexpr (std::is_void_v<std::invoke_result_t<F, const T&>>)
			{
				std::invoke(op, *ptr);

				if (rtime == vm::reservation_acquire(addr))
				{
					return;
				}
			}
			else
			{
				auto res = std::invoke(op, *ptr);

				if (rtime == vm::reservation_acquire(addr))
				{
					return res;
				}
			}
		}
	}

	template <bool Ack = false, typename T, typename F>
	inline SAFE_BUFFERS(auto) light_op(T& data, F op)
	{
		// Optimized real ptr -> vm ptr conversion, simply UB if out of range
		const u32 addr = static_cast<u32>(reinterpret_cast<const u8*>(&data) - g_base_addr);

		// Use "super" pointer to prevent access violation handling during atomic op
		const auto sptr = vm::get_super_ptr<T>(addr);

		// "Lock" reservation
		auto& res = vm::reservation_acquire(addr);

		auto [_old, _ok] = res.fetch_op([&](u64& r)
		{
			if (r & vm::rsrv_unique_lock)
			{
				return false;
			}

			r += 1;
			return true;
		});

		if (!_ok) [[unlikely]]
		{
			vm::reservation_shared_lock_internal(res);
		}

		if constexpr (std::is_void_v<std::invoke_result_t<F, T&>>)
		{
			std::invoke(op, *sptr);
			res += 127;

			if constexpr (Ack)
			{
				res.notify_all();
			}
		}
		else
		{
			auto result = std::invoke(op, *sptr);
			res += 127;

			if constexpr (Ack)
			{
				res.notify_all();
			}

			return result;
		}
	}

	template <bool Ack = false, typename T, typename F>
	inline SAFE_BUFFERS(auto) atomic_op(T& data, F op)
	{
		return light_op<Ack, T>(data, [&](T& data)
		{
			return data.atomic_op(op);
		});
	}

	template <bool Ack = false, typename T, typename F>
	inline SAFE_BUFFERS(auto) fetch_op(T& data, F op)
	{
		return light_op<Ack, T>(data, [&](T& data)
		{
			return data.fetch_op(op);
		});
	}
} // namespace vm
