#pragma once

#include "vm.h"
#include "vm_locking.h"
#include "Utilities/cond.h"
#include "util/atomic.hpp"
#include <functional>

extern bool g_use_rtm;

namespace vm
{
	enum reservation_lock_bit : u64
	{
		stcx_lockb = 1 << 0, // Exclusive conditional reservation lock
		dma_lockb = 1 << 5, // Exclusive unconditional reservation lock
		putlluc_lockb = 1 << 6, // Exclusive unconditional reservation lock
	};

	// Get reservation status for further atomic update: last update timestamp
	inline atomic_t<u64>& reservation_acquire(u32 addr, u32 size)
	{
		// Access reservation info: stamp and the lock bit
		return *reinterpret_cast<atomic_t<u64>*>(g_reservations + (addr & 0xff80) / 2);
	}

	// Update reservation status
	inline std::pair<bool, u64> try_reservation_update(u32 addr, u32 size, bool lsb = false)
	{
		// Update reservation info with new timestamp
		auto& res = reservation_acquire(addr, size);
		const u64 rtime = res;

		return {!(rtime & 127) && res.compare_and_swap_test(rtime, rtime + 128), rtime};
	}

	void reservation_update(u32 addr, u32 size, bool lsb = false);

	// Get reservation sync variable
	inline atomic_t<u64>& reservation_notifier(u32 addr, u32 size)
	{
		return *reinterpret_cast<atomic_t<u64>*>(g_reservations + (addr & 0xff80) / 2);
	}

	u64 reservation_lock_internal(u32, atomic_t<u64>&, u64);

	inline bool reservation_trylock(atomic_t<u64>& res, u64 rtime, u64 lock_bits = stcx_lockb)
	{
		if (res.compare_and_swap_test(rtime, rtime + lock_bits)) [[likely]]
		{
			return true;
		}

		return false;
	}

	inline std::pair<atomic_t<u64>&, u64> reservation_lock(u32 addr, u32 size, u64 lock_bits = stcx_lockb)
	{
		auto res = &vm::reservation_acquire(addr, size);
		auto rtime = res->load();

		if (rtime & 127 || !reservation_trylock(*res, rtime, lock_bits)) [[unlikely]]
		{
			static atomic_t<u64> no_lock{};

			rtime = reservation_lock_internal(addr, *res, lock_bits);

			if (rtime == umax)
			{
				res = &no_lock;
			}
		}

		return {*res, rtime};
	}

	void reservation_op_internal(u32 addr, std::function<bool()> func);

	template <typename T, typename AT = u32, typename F>
	SAFE_BUFFERS inline auto reservation_op(_ptr_base<T, AT> ptr, F op)
	{
		// Atomic operation will be performed on aligned 128 bytes of data, so the data size and alignment must comply
		static_assert(sizeof(T) <= 128 && alignof(T) == sizeof(T), "vm::reservation_op: unsupported type");
		static_assert(std::is_trivially_copyable_v<T>, "vm::reservation_op: not triv copyable (optimization)");

		// Use "super" pointer to prevent access violation handling during atomic op
		const auto sptr = vm::get_super_ptr<T>(static_cast<u32>(ptr.addr()));

		// Use 128-byte aligned addr
		const u32 addr = static_cast<u32>(ptr.addr()) & -128;

		if (g_use_rtm)
		{
			auto& res = vm::reservation_acquire(addr, 128);

			// Stage 1: single optimistic transaction attempt
			unsigned status = _XBEGIN_STARTED;

#ifndef _MSC_VER
			__asm__ goto ("xbegin %l[stage2];" ::: "memory" : stage2);
#else
			status = _xbegin();
			if (status == _XBEGIN_STARTED)
#endif
			{
				if constexpr (std::is_void_v<std::invoke_result_t<F, T&>>)
				{
					res += 128;
					std::invoke(op, *sptr);
#ifndef _MSC_VER
					__asm__ volatile ("xend;" ::: "memory");
#else
					_xend();
#endif
					res.notify_all();
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
						res.notify_all();
						return result;
					}
					else
					{
#ifndef _MSC_VER
						__asm__ volatile ("xabort $1;" ::: "memory");
#else
						_xabort(1);
#endif
						// Unreachable code
						return std::invoke_result_t<F, T&>();
					}
				}
			}

			stage2:
#ifndef _MSC_VER
			__asm__ volatile ("movl %%eax, %0;" : "=r" (status) :: "memory");
#endif
			if constexpr (!std::is_void_v<std::invoke_result_t<F, T&>>)
			{
				if (_XABORT_CODE(status))
				{
					// Unfortunately, actual function result is not recoverable in this case
					return std::invoke_result_t<F, T&>();
				}
			}

			// Touch memory if transaction failed without RETRY flag on the first attempt (TODO)
			if (!(status & _XABORT_RETRY))
			{
				reinterpret_cast<atomic_t<u8>*>(sptr)->fetch_add(0);
			}

			// Stage 2: try to lock reservation first
			res += stcx_lockb;

			// Start lightened transaction (TODO: tweaking)
			while (true)
			{
#ifndef _MSC_VER
				__asm__ goto ("xbegin %l[retry];" ::: "memory" : retry);
#else
				status = _xbegin();

				if (status != _XBEGIN_STARTED) [[unlikely]]
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
					res.notify_all();
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
						res.notify_all();
						return result;
					}
					else
					{
#ifndef _MSC_VER
						__asm__ volatile ("xabort $1;" ::: "memory");
#else
						_xabort(1);
#endif
						return std::invoke_result_t<F, T&>();
					}
				}

				retry:
#ifndef _MSC_VER
				__asm__ volatile ("movl %%eax, %0;" : "=r" (status) :: "memory");
#endif
				if (!(status & _XABORT_RETRY)) [[unlikely]]
				{
					if constexpr (!std::is_void_v<std::invoke_result_t<F, T&>>)
					{
						if (_XABORT_CODE(status))
						{
							res -= 1;
							return std::invoke_result_t<F, T&>();
						}
					}

					break;
				}
			}

			// Stage 3: all failed, heavyweight fallback (see comments at the bottom)
			if constexpr (std::is_void_v<std::invoke_result_t<F, T&>>)
			{
				return vm::reservation_op_internal(addr, [&]
				{
					std::invoke(op, *sptr);
					return true;
				});
			}
			else
			{
				auto result = std::invoke_result_t<F, T&>();

				vm::reservation_op_internal(addr, [&]
				{
					T buf = *sptr;

					if ((result = std::invoke(op, buf)))
					{
						*sptr = buf;
						return true;
					}
					else
					{
						return false;
					}
				});

				return result;
			}
		}


		// Perform under heavyweight lock
		auto& res = vm::reservation_acquire(addr, 128);

		res += stcx_lockb;

		// Write directly if the op cannot fail
		if constexpr (std::is_void_v<std::invoke_result_t<F, T&>>)
		{
			{
				vm::writer_lock lock(addr);
				std::invoke(op, *sptr);
				res += 127;
			}

			res.notify_all();
			return;
		}
		else
		{
			// Make an operational copy of data (TODO: volatile storage?)
			auto result = std::invoke_result_t<F, T&>();

			{
				vm::writer_lock lock(addr);
				T buf = *sptr;

				if ((result = std::invoke(op, buf)))
				{
					// If operation succeeds, write the data back
					*sptr = buf;
					res += 127;
				}
				else
				{
					// Operation failed, no memory has been modified
					res -= 1;
					return std::invoke_result_t<F, T&>();
				}
			}

			res.notify_all();
			return result;
		}
	}

	// For internal usage
	void reservation_escape_internal();

	// Read memory value in pseudo-atomic manner
	template <typename CPU, typename T, typename AT = u32, typename F>
	SAFE_BUFFERS inline auto reservation_peek(CPU&& cpu, _ptr_base<T, AT> ptr, F op)
	{
		// Atomic operation will be performed on aligned 128 bytes of data, so the data size and alignment must comply
		static_assert(sizeof(T) <= 128 && alignof(T) == sizeof(T), "vm::reservation_peek: unsupported type");

		// Use "super" pointer to prevent access violation handling during atomic op
		const auto sptr = vm::get_super_ptr<const T>(static_cast<u32>(ptr.addr()));

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

			const u64 rtime = vm::reservation_acquire(addr, 128);

			if (rtime & 127)
			{
				continue;
			}

			// Observe data non-atomically and make sure no reservation updates were made
			if constexpr (std::is_void_v<std::invoke_result_t<F, const T&>>)
			{
				std::invoke(op, *sptr);

				if (rtime == vm::reservation_acquire(addr, 128))
				{
					return;
				}
			}
			else
			{
				auto res = std::invoke(op, *sptr);

				if (rtime == vm::reservation_acquire(addr, 128))
				{
					return res;
				}
			}
		}
	}
} // namespace vm
