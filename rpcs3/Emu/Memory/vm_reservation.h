#pragma once

#include "vm.h"
#include "vm_locking.h"
#include "util/atomic.hpp"
#include <functional>

extern bool g_use_rtm;
extern u64 g_rtm_tx_limit2;

#ifdef _MSC_VER
extern "C"
{
	u64 __rdtsc();
	u32 _xbegin();
	void _xend();
}
#endif

namespace vm
{
	inline u64 get_tsc()
	{
#ifdef _MSC_VER
		return __rdtsc();
#else
		return __builtin_ia32_rdtsc();
#endif
	}

	enum : u64
	{
		rsrv_lock_mask = 127,
		rsrv_unique_lock = 64,
		rsrv_shared_mask = 63,
	};

	// Get reservation status for further atomic update: last update timestamp
	inline atomic_t<u64>& reservation_acquire(u32 addr)
	{
		// Access reservation info: stamp and the lock bit
		return *reinterpret_cast<atomic_t<u64>*>(g_reservations + (addr & 0xff80) / 2);
	}

	// Update reservation status
	void reservation_update(u32 addr);

	// Get reservation sync variable
	inline atomic_t<u64>& reservation_notifier(u32 addr)
	{
		return *reinterpret_cast<atomic_t<u64>*>(g_reservations + (addr & 0xff80) / 2);
	}

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
	inline SAFE_BUFFERS(auto) reservation_op(CPU& cpu, _ptr_base<T, AT> ptr, F op)
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

		if (g_use_rtm)
		{
			// Stage 1: single optimistic transaction attempt
			unsigned status = -1;
			u64 _old = 0;

			auto stamp0 = get_tsc(), stamp1 = stamp0, stamp2 = stamp0;

#ifndef _MSC_VER
			__asm__ goto ("xbegin %l[stage2];" ::: "memory" : stage2);
#else
			status = _xbegin();
			if (status == umax)
#endif
			{
				if (res & rsrv_unique_lock)
				{
#ifndef _MSC_VER
					__asm__ volatile ("xend; mov $-1, %%eax;" ::: "memory");
#else
					_xend();
#endif
					goto stage2;
				}

				if constexpr (std::is_void_v<std::invoke_result_t<F, T&>>)
				{
					std::invoke(op, *sptr);
					res += 128;
#ifndef _MSC_VER
					__asm__ volatile ("xend;" ::: "memory");
#else
					_xend();
#endif
					if constexpr (Ack)
						res.notify_all(-128);
					return;
				}
				else
				{
					if (auto result = std::invoke(op, *sptr))
					{
						res += 128;
#ifndef _MSC_VER
						__asm__ volatile ("xend;" ::: "memory");
#else
						_xend();
#endif
						if constexpr (Ack)
							res.notify_all(-128);
						return result;
					}
					else
					{
#ifndef _MSC_VER
						__asm__ volatile ("xend;" ::: "memory");
#else
						_xend();
#endif
						return result;
					}
				}
			}

			stage2:
#ifndef _MSC_VER
			__asm__ volatile ("mov %%eax, %0;" : "=r" (status) :: "memory");
#endif
			stamp1 = get_tsc();

			// Stage 2: try to lock reservation first
			_old = res.fetch_add(1);

			// Compute stamps excluding memory touch
			stamp2 = get_tsc() - (stamp1 - stamp0);

			// Start lightened transaction
			for (; !(_old & vm::rsrv_unique_lock) && stamp2 - stamp0 <= g_rtm_tx_limit2; stamp2 = get_tsc())
			{
				if (cpu.has_pause_flag())
				{
					break;
				}

#ifndef _MSC_VER
				__asm__ goto ("xbegin %l[retry];" ::: "memory" : retry);
#else
				status = _xbegin();

				if (status != umax) [[unlikely]]
				{
					goto retry;
				}
#endif
				if constexpr (std::is_void_v<std::invoke_result_t<F, T&>>)
				{
					std::invoke(op, *sptr);
#ifndef _MSC_VER
					__asm__ volatile ("xend;" ::: "memory");
#else
					_xend();
#endif
					res += 127;
					if (Ack)
						res.notify_all(-128);
					return;
				}
				else
				{
					if (auto result = std::invoke(op, *sptr))
					{
#ifndef _MSC_VER
						__asm__ volatile ("xend;" ::: "memory");
#else
						_xend();
#endif
						res += 127;
						if (Ack)
							res.notify_all(-128);
						return result;
					}
					else
					{
#ifndef _MSC_VER
						__asm__ volatile ("xend;" ::: "memory");
#else
						_xend();
#endif
						return result;
					}
				}

				retry:
#ifndef _MSC_VER
				__asm__ volatile ("mov %%eax, %0;" : "=r" (status) :: "memory");
#endif

				if (!status)
				{
					break;
				}
			}

			// Stage 3: all failed, heavyweight fallback (see comments at the bottom)
			if constexpr (std::is_void_v<std::invoke_result_t<F, T&>>)
			{
				vm::reservation_op_internal(addr, [&]
				{
					std::invoke(op, *sptr);
					return true;
				});

				if constexpr (Ack)
					res.notify_all(-128);
				return;
			}
			else
			{
				auto result = std::invoke_result_t<F, T&>();

				vm::reservation_op_internal(addr, [&]
				{
					if ((result = std::invoke(op, *sptr)))
					{
						return true;
					}
					else
					{
						return false;
					}
				});

				if (Ack && result)
					res.notify_all(-128);
				return result;
			}
		}

		// Lock reservation and perform heavyweight lock
		reservation_shared_lock_internal(res);

		if constexpr (std::is_void_v<std::invoke_result_t<F, T&>>)
		{
			{
				vm::writer_lock lock(addr);
				std::invoke(op, *sptr);
				res += 127;
			}

			if constexpr (Ack)
				res.notify_all(-128);
			return;
		}
		else
		{
			auto result = std::invoke_result_t<F, T&>();
			{
				vm::writer_lock lock(addr);

				if ((result = std::invoke(op, *sptr)))
				{
					res += 127;
				}
				else
				{
					res -= 1;
				}
			}

			if (Ack && result)
				res.notify_all(-128);
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
				res.notify_all(-128);
			}
		}
		else
		{
			auto result = std::invoke(op, *sptr);
			res += 127;

			if constexpr (Ack)
			{
				res.notify_all(-128);
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
